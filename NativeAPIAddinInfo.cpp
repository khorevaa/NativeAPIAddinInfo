// NativeAPIAddinInfo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <windows.h>
#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

typedef long(_cdecl *f_GetClassObject)(const WCHAR_T* wsName, IComponentBase** pInterface);
typedef WCHAR_T*(_cdecl *f_GetClassNames)();


class MyMemoryManager: public IMemoryManager
{
public:
	MyMemoryManager(void) {};
	virtual ~MyMemoryManager() {};
	/// Allocates memory of specified size
	 /**
	 *  @param pMemory - the double pointer to variable, that will hold newly
	 *      allocated block of memory of NULL if allocation fails.
	 *  @param ulCountByte - memory size
	 *  @return the result of
	 */
	virtual bool ADDIN_API AllocMemory(void** pMemory, unsigned long ulCountByte) {
		*pMemory = new byte[ulCountByte];
		memset(*pMemory, 0, ulCountByte);
		return true;
	};
	/// Releases memory
	 /**
	 *  @param pMemory - The double pointer to the memory block being released
	 */
	virtual void ADDIN_API FreeMemory(void** pMemory) {
		delete[] * pMemory;
	};
};

bool GetParamDefValue(IComponentBase *obj, const long lMethodNum, const long lParamNum,
	wstring &str) {
	tVariant pvarParamDefValue;
	if (obj->GetParamDefValue(lMethodNum, lParamNum, &pvarParamDefValue)) {
		if (TV_VT(&pvarParamDefValue) == VTYPE_PWSTR) {
			if (pvarParamDefValue.wstrLen == 0) {
				str = L"\"\"";
			}
			else
				str.assign(pvarParamDefValue.pwstrVal, pvarParamDefValue.wstrLen);
			return true;
		}
		else if (TV_VT(&pvarParamDefValue) == VTYPE_PSTR) {
			string tstr;
			if (pvarParamDefValue.strLen == 0)
				tstr = "\"\"";
			else
				tstr.assign(pvarParamDefValue.pstrVal, pvarParamDefValue.strLen);
			wchar_t *StrValue = new wchar_t[tstr.size() + 1];
			memset(StrValue, 0, (tstr.size() + 1) * sizeof(wchar_t));
			std::mbstowcs(StrValue, tstr.c_str(), tstr.size() * sizeof(WCHAR_T));
			str.assign(StrValue);
			delete[] StrValue;
			return true;
		}
		else if (TV_VT(&pvarParamDefValue) == VTYPE_BOOL) {
			if (pvarParamDefValue.bVal == true)
				str = L"True";
			else
				str = L"False";
		}
		else if (TV_VT(&pvarParamDefValue) == VTYPE_INT) {
			wchar_t* StrValue = new wchar_t[_MAX_U64TOSTR_BASE2_COUNT + 1];
			memset(StrValue, 0, (_MAX_U64TOSTR_BASE2_COUNT + 1) * sizeof(wchar_t));
			_itow(pvarParamDefValue.intVal, StrValue, 10);
			str.assign(StrValue);
			delete[] StrValue;
			return true;
		}
		else if (TV_VT(&pvarParamDefValue) == VTYPE_R8 || TV_VT(&pvarParamDefValue) == VTYPE_R4) {
			str = to_wstring(pvarParamDefValue.dblVal);			
			return true;
		}

	}
	return false;
}


int main(int argc, const char * argv[])
{
	setlocale(LC_ALL, "ru_RU");

	if (argc <= 1) {
		cout << "Please, specify path to the file of the component in the first parameter." << std::endl;
		return -1;
	}

	DWORD dwAttrib = GetFileAttributesA(argv[1]);
	if (!(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
	{
		cout << "File not found." << std::endl;
		return -1;
	}

	HINSTANCE hGetProcIDDLL = LoadLibraryA(argv[1]);

	if (!hGetProcIDDLL) {
		cout << "Could not load the dynamic library" << std::endl;
		return EXIT_FAILURE;
	}

	// resolve function address here
	f_GetClassObject GetClassObject = (f_GetClassObject)GetProcAddress(hGetProcIDDLL, "GetClassObject");
	if (!GetClassObject) {
		cout << "Could not locate the function GetClassObject()" << std::endl;
		return EXIT_FAILURE;
	}

	f_GetClassNames GetClassNames = (f_GetClassNames)GetProcAddress(hGetProcIDDLL, "GetClassNames");
	if (!GetClassNames) {
		cout << "Could not locate the function GetClassNames()" << std::endl;
		return EXIT_FAILURE;
	}

	wstring objNames = GetClassNames();

	wcout << "Available object names:" << objNames << endl;

	wstring objName = L"";

	std::wstringstream ss(objNames);

	wchar_t delim = '|';
	while (std::getline(ss, objName, delim)) {
	
		if (objName.size() > 0) {
			wcout << "Get the properties of the object \"" << objName << "\":" << endl << endl;
		}

		IComponentBase* obj = NULL;

		if (GetClassObject(objName.c_str(), &obj) == 0 || obj == NULL)
		{
			wcout << L"Could not get the object \"" << objName << L"\""<< std::endl;
			continue;
		}

		long nmethods = obj->GetNMethods();
		cout << "Methods: " << std::endl;

		MyMemoryManager mem;

		obj->setMemManager(&mem);

		for (int i = 0; i < nmethods; i++) {

			wcout << "------------------------------------------------" << std::endl;

			wcout << L"   " << obj->GetMethodName(i, 0) << L"(";

			long nparams = obj->GetNParams(i);
			for (long ParamNum = 0; ParamNum < nparams; ParamNum++)
			{
				wcout << L"p" << ParamNum + 1;

				wstring str;
				if (GetParamDefValue(obj, i, ParamNum, str))
				{
					wcout << L"=" << str;
				}

				if (ParamNum < (nparams - 1))
					wcout << ", ";
			}
			wcout << ")" << std::endl;

			wcout << L"   local name: " << obj->GetMethodName(i, 1) << std::endl;

			if (obj->HasRetVal(i))
				wcout << L"   Has return value: " << L"True" << std::endl;
			else
				wcout << L"   Has return value: " << L"False" << std::endl;
		}

		long nprops = obj->GetNProps();
		std::cout << std::endl << "Properties: " << std::endl;

		for (int i = 0; i < nprops; i++) {
			wcout << "------------------------------------------------" << std::endl;
			wcout << L"   " << obj->GetPropName(i, 0) << std::endl;;
			wcout << L"   local name: " << obj->GetPropName(i, 1) << std::endl;
			if (obj->IsPropReadable(i))
				wcout << L"   Readable: True" << std::endl;
			else
				wcout << L"   Readable: False" << std::endl;
			if (obj->IsPropWritable(i))
				wcout << L"   Writable: True" << std::endl;
			else
				wcout << L"   Writable: False" << std::endl;

		}
	}

	return EXIT_SUCCESS;
}

