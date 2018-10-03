#include <atlbase.h>
#include <PortableDeviceApi.h>
#include <PortableDevice.h>

#define NUM_OBJECTS_TO_REQUEST  1000

HRESULT hr;
CComPtr<IPortableDeviceContent>  content;

bool FindDeviceContext(PWSTR match)
{
	CComPtr<IPortableDeviceManager> deviceManager;
	hr = CoCreateInstance(CLSID_PortableDeviceManager, NULL, CLSCTX_INPROC_SERVER, IID_IPortableDeviceManager, (VOID**)&deviceManager);

	DWORD pnpDeviceIDCount = 0;
	hr = deviceManager->GetDevices(NULL, &pnpDeviceIDCount);

	PWSTR* pnpDeviceIDs = new PWSTR[pnpDeviceIDCount];
	ZeroMemory(pnpDeviceIDs, pnpDeviceIDCount * sizeof(PWSTR));
	hr = deviceManager->GetDevices(pnpDeviceIDs, &pnpDeviceIDCount);

	PWSTR device_id = NULL;
	for (DWORD i = 0; i < pnpDeviceIDCount; i++)
	{
		device_id = pnpDeviceIDs[i];

		DWORD len = 0;
		deviceManager->GetDeviceFriendlyName(device_id, 0, &len);
		PWSTR friendlyName = new WCHAR[len];
		deviceManager->GetDeviceFriendlyName(device_id, friendlyName, &len);

		if (wcscmp(friendlyName, match) == 0)
			break;
	}

	if (device_id == NULL)
		return false;

	CComPtr<IPortableDevice> device;
	hr = CoCreateInstance(CLSID_PortableDeviceFTM, nullptr, CLSCTX_INPROC_SERVER, IID_IPortableDevice, (VOID**)&device);

	CComPtr<IPortableDeviceValues> clientInfo;
	hr = CoCreateInstance(CLSID_PortableDeviceValues, NULL, CLSCTX_INPROC_SERVER, IID_IPortableDeviceValues, (VOID**)&clientInfo);
	clientInfo->SetStringValue(WPD_CLIENT_NAME, L"FooBar");
	clientInfo->SetUnsignedIntegerValue(WPD_CLIENT_MAJOR_VERSION, 1);
	clientInfo->SetUnsignedIntegerValue(WPD_CLIENT_MINOR_VERSION, 0);
	clientInfo->SetUnsignedIntegerValue(WPD_CLIENT_REVISION, 0);
	clientInfo->SetUnsignedIntegerValue(WPD_CLIENT_SECURITY_QUALITY_OF_SERVICE, SECURITY_IMPERSONATION);

	device->Open(device_id, clientInfo);

	hr = device->Content(&content);

	return true;
}

PWSTR GetName(PWSTR objectID)
{
	CComPtr<IPortableDeviceKeyCollection>  propertiesToRead;
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection, nullptr, CLSCTX_INPROC_SERVER, IID_IPortableDeviceKeyCollection, (VOID**)&propertiesToRead);
	hr = propertiesToRead->Add(WPD_OBJECT_NAME);

	CComPtr<IPortableDeviceProperties> properties;
	hr = content->Properties(&properties);

	CComPtr<IPortableDeviceValues> objectProperties;
	hr = properties->GetValues(objectID, propertiesToRead, &objectProperties);

	PWSTR name = nullptr;
	hr = objectProperties->GetStringValue(WPD_OBJECT_NAME, &name);

	return name;
}

DATE GetDate(PWSTR objectID)
{
	CComPtr<IPortableDeviceKeyCollection>  propertiesToRead;
	hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection, nullptr, CLSCTX_INPROC_SERVER, IID_IPortableDeviceKeyCollection, (VOID**)&propertiesToRead);
	hr = propertiesToRead->Add(WPD_OBJECT_DATE_MODIFIED);

	CComPtr<IPortableDeviceProperties> properties;
	hr = content->Properties(&properties);

	CComPtr<IPortableDeviceValues> objectProperties;
	hr = properties->GetValues(objectID, propertiesToRead, &objectProperties);

	PROPVARIANT last_modified_date = { 0 };
	PropVariantInit(&last_modified_date);
	hr = objectProperties->GetValue(WPD_OBJECT_DATE_MODIFIED, &last_modified_date);

	return last_modified_date.date;
}

PWSTR GetContent(PCWSTR parent_id, PWSTR target)
{
	CComPtr<IEnumPortableDeviceObjectIDs> enumObjectIDs;
	hr = content->EnumObjects(0, parent_id, nullptr, &enumObjectIDs);

	while (hr == S_OK)
	{
		PWSTR  objectIDArray[NUM_OBJECTS_TO_REQUEST] = { 0 };
		DWORD  numFetched = 0;
		hr = enumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST, objectIDArray, &numFetched);

		for (DWORD i = 0; i < numFetched; i++)
		{
			PWSTR child_id = objectIDArray[i];
			PWSTR name = GetName(child_id);

			if (wcscmp(name, target) == 0)
				return child_id;
		}
	}

	return NULL;
}

PWSTR GetLatestContent(PCWSTR parent_id, PWSTR target)
{
	CComPtr<IEnumPortableDeviceObjectIDs> enumObjectIDs;
	hr = content->EnumObjects(0, parent_id, nullptr, &enumObjectIDs);

	DATE latestDate = 0;
	PWSTR latestFile = NULL;

	while (hr == S_OK)
	{
		PWSTR  objectIDArray[NUM_OBJECTS_TO_REQUEST] = { 0 };
		DWORD  numFetched = 0;
		hr = enumObjectIDs->Next(NUM_OBJECTS_TO_REQUEST, objectIDArray, &numFetched);

		for (DWORD i = 0; i < numFetched; i++)
		{
			PWSTR child_id = objectIDArray[i];
			PWSTR name = GetName(child_id);
			DATE modified = GetDate(child_id);

			if (wcsncmp(name, target, wcslen(target)) == 0 && modified > latestDate)
			{
				latestDate = modified;
				latestFile = child_id;
			}
		}
	}

	return latestFile;
}

void Upload(PWSTR objectID, PWSTR destFilename)
{
	CComPtr<IPortableDeviceResources>  resources;
	hr = content->Transfer(&resources);

	DWORD optimalTransferSizeBytes = 0;
	CComPtr<IStream> objectDataStream;
	hr = resources->GetStream(objectID, WPD_RESOURCE_DEFAULT, STGM_READ, &optimalTransferSizeBytes, &objectDataStream);

	CComPtr<IStream> finalFileStream;
	hr = SHCreateStreamOnFileEx(destFilename, STGM_CREATE | STGM_WRITE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &finalFileStream);

	BYTE* objectData = new BYTE[optimalTransferSizeBytes];
	while (1)
	{
		DWORD bytesRead = 0;
		hr = objectDataStream->Read(objectData, optimalTransferSizeBytes, &bytesRead);

		if (bytesRead == 0)
			break;

		DWORD bytesWritten = 0;
		hr = finalFileStream->Write(objectData, bytesRead, &bytesWritten);
	}
}

extern "C" __declspec(dllexport) bool UploadHeats(PWSTR destFile)
{
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	bool connected = FindDeviceContext(L"Lenovo TAB 2 A10-30");

	if (!connected)
		return false;

	PWSTR InternalStorage = GetContent(WPD_DEVICE_OBJECT_ID, L"Internal storage");
	if (!InternalStorage)
		return false;

	PWSTR Download = GetContent(InternalStorage, L"Download");
	if (!Download)
		return false;

	PWSTR File = GetLatestContent(Download, L"heats_");
	if (!File)
		return false;

	Upload(File, destFile);

	return true;
}
