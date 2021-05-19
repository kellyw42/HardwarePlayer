#pragma once

static ReportSync report_sync;
static BangHandler bang_handler;

static long *start_bangs = NULL, *finish_bangs = NULL;
static long start_count = 0, finish_count = 0;

static HANDLE new_start_bang, new_finish_bang;

static int align_length;
static int exclude[10000];
static long align_start[10000];
static long align_finish[10000];

static FILE *logg;

extern void new_bangs(int which, long* bangs, long count)
{
	if (which == 0)
	{
		start_bangs = bangs;
		start_count = count;
		SetEvent(new_start_bang);
	}
	else
	{
		finish_bangs = bangs;
		finish_count = count;
		SetEvent(new_finish_bang);
	}
}

static double ToSeconds(long audio_samples)
{
	return audio_samples / 48000.0;
}

static int Align(int i, int j)
{
	int length = 0;

	long diff = finish_bangs[i] - start_bangs[j];

	align_finish[length] = finish_bangs[i++];
	align_start[length] = start_bangs[j++];
	length++;

	while (i < finish_count && j < start_count)
	{
		if (ToSeconds(abs(finish_bangs[i] - start_bangs[j] - diff)) <= 0.2)
		{
			align_finish[length] = finish_bangs[i++];
			align_start[length] = start_bangs[j++];
			length++;
		}
		else if (finish_bangs[i] - start_bangs[j] - diff < 0)
			i++;
		else
			j++;
	}

	return length;
}

static void FindBestAlignment()
{
	int longest = 0;
	int best_i = -1, best_j = -1;
	for (int i = 0; i < 1 && i < finish_count; i++)
	{
		{
			for (int j = 0; j < 1 && j < start_count; j++)
			{
				{
					int align = Align(i, j);
					if (align > longest)
					{
						longest = align;
						best_i = i;
						best_j = j;
					}
				}
			}
		}
	}

	if (best_i >= 0 && best_j >= 0)
		align_length = Align(best_i, best_j);
}

static void LinearFit2()
{
	double cx = align_start[0];
	double cy = align_finish[0] - align_start[0];

	double total = 0;
	for (int i = 1; i < align_length; i++)
	{
		double m = ((double)(align_finish[i] - align_start[i] - cy)) / (align_start[i] - cx);
		total += m;
	}

	double m = total / align_length;

	m += 1.0;

	double maxError = 0;
	for (int i = 1; i < align_length; i++)
	{
		double error = (align_finish[i] - align_start[i] - cy) - (align_start[i] - cx) * m;
		if (abs(error) > maxError)
			maxError = abs(error);
	}

	report_sync(2, cx, cy, m);
}

static void LinearFit()
{
	int n = 0;
	double  sumX = 0.0;
	double sumY = 0.0;
	for (int i = 0; i < align_length; i++)
		if (exclude[i] == 0)
		{
			sumX += align_start[i];
			sumY += (align_finish[i] - align_start[i]);
			n++;
		}
	double avgX = ((double)sumX) / n;
	double avgY = ((double)sumY) / n;

	double top = 0.0;
	double bottom = 0.0;
	for (int i = 0; i < align_length; i++)
		if (exclude[i] == 0)
		{
			top += (align_start[i] - avgX) * (align_finish[i] - align_start[i] - avgY);
			bottom += (align_start[i] - avgX) * (align_start[i] - avgX);
		}

	double c1 = top / bottom;
	double c0 = avgY - c1 * avgX;

	double maxError = 0;
	int outlier = 0; ;
	for (int i = 1; i < align_length; i++)
		if (exclude[i] == 0)
		{
			double error = abs((align_start[i] * c1 + c0) - (align_finish[i] - align_start[i]));
			if (error > maxError)
			{
				maxError = error;
				outlier = i;
			}
		}

	if (maxError > 50)
	{
		exclude[outlier] = 1;
		LinearFit();
	}
	else
	{
		report_sync(2, align_start[0], c0, c1);
	}
}


static void Analyse()
{
	align_length = 0;
	FindBestAlignment();

	if (align_length > 1)
	{
		memset(exclude, 0, sizeof(exclude));
		LinearFit();
	}
}

DWORD WINAPI SyncProc(LPVOID lpThreadParameter)
{
	HANDLE more_bangs[2] = { new_start_bang, new_finish_bang };
	WaitForMultipleObjects(2, more_bangs, true, INFINITE);
	Analyse();
	return 0;
}

extern void StartSync(ReportSync sync_handler, BangHandler bang_handler0)
{
	report_sync = sync_handler;
	bang_handler = bang_handler0;
	new_start_bang = CreateEvent(0, FALSE, FALSE, 0);
	new_finish_bang = CreateEvent(0, FALSE, FALSE, 0);

	CreateThread(0, 0, SyncProc, 0, 0, 0);
}
