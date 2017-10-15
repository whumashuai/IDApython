#include "stdafx.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cwchar>
#include <dia2.h>
#include <atlbase.h>
#include <comutil.h>

#define Z_DBG 1

using std::printf;

const wchar_t *SymTagStrs[] = {
	L"SymTagNull",
	L"SymTagExe",
	L"SymTagCompiland",
	L"SymTagCompilandDetails",
	L"SymTagCompilandEnv",
	L"SymTagFunction",
	L"SymTagBlock",
	L"SymTagData",
	L"SymTagAnnotation",
	L"SymTagLabel",
	L"SymTagPublicSymbol",
	L"SymTagUDT",
	L"SymTagEnum",
	L"SymTagFunctionType",
	L"SymTagPointerType",
	L"SymTagArrayType",
	L"SymTagBaseType",
	L"SymTagTypedef",
	L"SymTagBaseClass",
	L"SymTagFriend",
	L"SymTagFunctionArgType",
	L"SymTagFuncDebugStart",
	L"SymTagFuncDebugEnd",
	L"SymTagUsingNamespace",
	L"SymTagVTableShape",
	L"SymTagVTable",
	L"SymTagCustom",
	L"SymTagThunk",
	L"SymTagCustomType",
	L"SymTagManagedType",
	L"SymTagDimension",
	L"SymTagCallSite",
	L"SymTagInlineSite",
	L"SymTagBaseInterface",
	L"SymTagVectorType",
	L"SymTagMatrixType",
	L"SymTagHLSLType"
};
const wchar_t *LocationTypeStrs[] = {
	L"LocIsNull",
	L"LocIsStatic",
	L"LocIsTLS",
	L"LocIsRegRel",
	L"LocIsThisRel",
	L"LocIsEnregistered",
	L"LocIsBitField",
	L"LocIsSlot",
	L"LocIsIlRel",
	L"LocInMetaData",
	L"LocIsConstant",
	L"LocTypeMax"
};

const wchar_t *BasicTypeEnumToStr(BasicType bt, ULONGLONG size) {
	switch (bt) {
	case btNoType:
		return L"btNoType";
		break;
	case btVoid:
		return L"void";
		break;
	case btChar:
		return L"char";
		break;
	case btWChar:
		return L"wchar_t";
		break;
	case btInt:
		if (size == 2)
			return L"short int";
		if (size == 4)
			return L"int";
		if (size == 8)
			return L"long long int";
	case btUInt:
		if (size == 2)
			return L"unsigned short int";
		if (size == 4)
			return L"unsigned int";
		if (size == 8)
			return L"unsigned long long int";
	case btFloat:
		if (size == 4)
			return L"float";
		if (size == 8)
			return L"double";
	case btBCD:
		return L"BCD";
		break;
	case btBool:
		return L"bool";
		break;
	case btLong:
		return L"long";
		break;
	case btULong:
		return L"unsigned long";
		break;
	case btCurrency:
		return L"btCurrency";
		break;
	case btDate:
		return L"btDate";
		break;
	case btVariant:
		return L"btVariant";
		break;
	case btComplex:
		return L"btComplex";
		break;
	case btBit:
		return L"btBit";
		break;
	case btBSTR:
		return L"btBSTR";
		break;
	case btHresult:
		return L"btHresult";
		break;
	case btChar16:
		return L"btChar16";
		break;
	case btChar32:
		return L"btChar32";
		break;
	default:
		return L"default baseType";
		break;
	}
}

// Class
class CDiaBSTR {
	BSTR m_bstr;
public:
	CDiaBSTR(const wchar_t *str, size_t times) {
		std::size_t length = std::wcslen(str) * times;
		m_bstr = SysAllocStringLen(nullptr, length);
		for (size_t i = 0; i < times; ++i) {
		}
	}
	CDiaBSTR() { m_bstr = nullptr; }
	~CDiaBSTR() { if (!isNull()) SysFreeString(m_bstr); }

	BSTR *operator &() { assert(isNull()); return &m_bstr; }
	operator BSTR() { assert(!isNull()); return m_bstr; }

	bool isNull() { return m_bstr == nullptr; }
	bool startWith(const wchar_t *prefix) {
		assert(!isNull());
		size_t n = std::wcslen(prefix);
		return std::wcsncmp(prefix, m_bstr, n) == 0;
	}
};

void Error(const char *msg);
void Fatal(const char *msg);
void DumpAll(char *szFilename, IDiaDataSource *pSource, wchar_t *szLookup);
void DumpSegments(IDiaEnumSegments *pSegments);
void DumpSectionContribs(IDiaEnumSectionContribs *pSectionContribs);
void DumpSymbols(IDiaEnumSymbols *pSymbols);

void PrintSymbolUDT(IDiaSymbol *pSymbol);
void PrintSymbolData(IDiaSymbol *pSymbol);

void PrintSymbolDataAddress(IDiaSymbol *pSym, LocationType LocTy);

void printType(IDiaSymbol *pType);

// Global variables
CComPtr<IDiaSession> psession;
CComPtr<IDiaSymbol> pglobal;

/********************************************/
int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <pdf-filename>\n", argv[0]);
		std::exit(EXIT_FAILURE);
	}

	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr)) {
		Fatal("CoInitialize failed");
	}

	CComPtr<IDiaDataSource> pSource;
	hr = CoCreateInstance(CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER,
		__uuidof(IDiaDataSource),
		reinterpret_cast<LPVOID *>(&pSource));

	if (FAILED(hr)) {
		Fatal("Could not CoCreate CLSID_DiaSource. Register msdia140.dll.");
	}

	// Do the real jobs
	DumpAll(argv[1], pSource, nullptr);

	pglobal = nullptr;
	psession = nullptr;
	pSource = nullptr;

	CoUninitialize();
	system("pause");

	return 0;
}

void Error(const char *msg) {
	printf("Error: %s\n", msg);
}

void Fatal(const char *msg) {
	printf("*** Fatal error: %s\n", msg);
	std::exit(EXIT_FAILURE);
}

void printType(IDiaSymbol *pType) {
	DWORD tag = 0;
	pType->get_symTag(&tag);

	CDiaBSTR name;
	if (pType->get_name(&name) == S_OK &&
		!name.isNull() &&
		std::wcslen(name) != 0) {
		printf("%ws", name);
	}
	else if (tag == SymTagPointerType) {
		CComPtr<IDiaSymbol> pBaseType;
		if (pType->get_type(&pBaseType) == S_OK) {
			printType(pBaseType);
			printf("*");
		}
		else {
			Fatal("pointer get_type");
		}
	}
	else if (tag == SymTagBaseType) {
		DWORD baseType;
		ULONGLONG size;

		if (pType->get_baseType(&baseType) == S_OK &&
			pType->get_length(&size) == S_OK) {
			printf("%ws", BasicTypeEnumToStr(static_cast<BasicType>(baseType),
				size));
		}
	}
	else if (tag == SymTagArrayType) {
		CComPtr<IDiaSymbol> pBaseType;
		if (pType->get_type(&pBaseType) == S_OK) {
			printf("arr(");
			printType(pBaseType);
		}
		else {
			Fatal("array get_type");
		}

		DWORD rank;
		DWORD celt;
		LONG count;
		CComPtr<IDiaEnumSymbols> pEnum;

		if (pType->get_count(&rank) == S_OK) {
			printf(", %lu)", rank);
		}
#if 0
		if (pType->get_rank(&rank) == S_OK) {
			if (pType->findChildren(SymTagDimension, nullptr, nsNone, &pEnum) == S_OK &&
				pEnum != NULL) {
				CComPtr<IDiaSymbol> pSym;
				while (pEnum->Next(1, &pSym, &celt) == S_OK && celt == 1) {
					CComPtr<IDiaSymbol> pBound;
					printf("[");
					if (pSym->get_lowerBound(&pBound) == S_OK) {
						printBound(pBound);
						printf("..");
					}
					pBound = NULL;
					if (pSym->get_upperBound(&pBound) == S_OK) {
						printBound(pBound);
					}
					pBound = NULL;
					printf("]");
					pSym = NULL;
				}
			}
		}
#endif
	}
	else {
		printf("%ws", SymTagStrs[tag]);
	}
}

#if 0
void printIsCompilerGenerated(IDiaSymbol *pSymbol) {
	BOOL flag;
	printf("Data Define:\t");

	switch (pSymbol->get_compilerGenerated(&flag)) {
	default:
		Fatal("unknown return value");
		break;
	case S_OK:
		puts(flag
			? "compiler-generated symbol"
			: "User-defined symbol");
		break;
	case S_FALSE:
		puts("get_compilerGenerated failed");
		break;
	}
}

void printFileOfSymbol(IDiaSymbol *pSymbol) {
	CComPtr<IDiaSymbol> pParent(nullptr), pSelf(pSymbol);
	printf("Hierarchy:   ");

	while (SUCCEEDED(pSelf->get_lexicalParent(&pParent)) && pParent != nullptr) {
		DWORD symTag;
		pSelf->get_symTag(&symTag);

		CDiaBSTR name;
		if (pSelf->get_name(&name) == S_OK && name != nullptr) {
			printf("%ws(T: %ws) -- ", name, SymTagStrs[symTag]);
		}
		else {
			continue;
			printf("noname(T:%ws) -- ", SymTagStrs[symTag]);
		}

		pSelf = pParent;
		pParent = nullptr;
	}

	printf("\n");
}
#endif

void DumpAll(char *szFilename, IDiaDataSource *pSource, wchar_t *szLookup) {
	HRESULT hr;
	wchar_t wszFilename[_MAX_PATH];

	mbstowcs(wszFilename, szFilename,
		sizeof(wszFilename) / sizeof(wszFilename[0]));

	if (FAILED(pSource->loadDataFromPdb(wszFilename)) &&
		FAILED(pSource->loadDataForExe(wszFilename, nullptr, nullptr))) {
		Fatal("loadDataFromPdb/Exe");
	}

	if (FAILED(pSource->openSession(&psession))) {
		Fatal("openSession");
	}
	ULONG celt = 0;

	CComPtr<IDiaEnumTables> pTables;
	if (FAILED(psession->getEnumTables(&pTables)))
		Fatal("getEnumTables");


	for (CComPtr<IDiaTable> pTable;
		SUCCEEDED(hr = pTables->Next(1, &pTable, &celt)) && celt == 1;
		pTable = nullptr) {

#if 0
		CDiaBSTR bstrTableName;
		if (pTable->get_name(&bstrTableName) != 0)
			Fatal("get_name");
		printf("\nFound table: %ws\n", bstrTableName);
#endif

		CComPtr<IDiaEnumSymbols> pSymbols;
		CComPtr<IDiaEnumSegments> pSegments;
		CComPtr<IDiaEnumSectionContribs> pSectionContribs;

		if (SUCCEEDED(pTable->QueryInterface(
			__uuidof(IDiaEnumSegments), reinterpret_cast<void **>(&pSegments)))) {
			DumpSegments(pSegments);
		}
		else if (SUCCEEDED(pTable->QueryInterface(
			__uuidof(IDiaEnumSectionContribs), reinterpret_cast<void **>(&pSectionContribs)))) {
			// We don't need this info
			//ShowSectionContribs(pSectionContribs);
		}
		else if (SUCCEEDED(pTable->QueryInterface(
			_uuidof(IDiaEnumSymbols), reinterpret_cast<void **>(&pSymbols)))) {
			DumpSymbols(pSymbols);
		}
		else {
			//printf("No action...\n");
		}
	}
	printf("Done\n");
}

void DumpSegments(IDiaEnumSegments *pSegments) {
	HRESULT hr = 0;
	ULONG celt = 0;
	CComPtr<IDiaSegment> pSegment;

	while (SUCCEEDED(hr = pSegments->Next(1, &pSegment, &celt)) && celt == 1) {
		DWORD rva, seg, frame, offset;
		ULONGLONG va;

		pSegment->get_addressSection(&seg);
		pSegment->get_frame(&frame);
		pSegment->get_offset(&offset);
		pSegment->get_virtualAddress(&va);

		if (pSegment->get_relativeVirtualAddress(&rva) == S_OK) {
			printf("VA: %llu ", va);
			printf("Frame: %i Segment %i addr: 0x%.8X offset: 0x%.8X\n", frame, seg, rva, offset);
			pSegment = NULL;

			CComPtr<IDiaSymbol> pSym;
			if (psession->findSymbolByRVA(rva, SymTagNull, &pSym) == S_OK) {
				CDiaBSTR name;
				DWORD    tag;

				pSym->get_symTag(&tag);
				pSym->get_name(&name);
				printf("\tClosest symbol: %ws (%ws)\n",
					name != NULL ? name : L"",
					SymTagStrs[tag]);
			}
		}
		else {
			printf("Segment %i \n", seg);
			pSegment = NULL;
			CComPtr<IDiaSymbol> pSym;
			if (SUCCEEDED(psession->findSymbolByAddr(seg, 0, SymTagNull, &pSym))) {
				CDiaBSTR name;
				DWORD tag;
				pSym->get_symTag(&tag);
				pSym->get_name(&name);
				printf("\tClosest symbol: %ws (%ws)\n", name != NULL ? name : L"", SymTagStrs[tag]);
			}
		}
	}
}

void DumpSectionContribs(IDiaEnumSectionContribs *pSectionContribs) {
	ULONG celt = 0;

	for (CComPtr<IDiaSectionContrib> pSectionContrib;
		pSectionContribs->Next(1, &pSectionContrib, &celt) == S_OK && celt == 1;
		pSectionContrib = nullptr) {
		DWORD rva, seg, addrOffset;
		ULONGLONG va;

		pSectionContrib->get_addressSection(&seg);
		pSectionContrib->get_virtualAddress(&va);
		pSectionContrib->get_addressOffset(&addrOffset);
		pSectionContrib->get_relativeVirtualAddress(&rva);

		printf("rva=%x seg=%x addrOffset=%x va=%llu\n", rva, seg, addrOffset, va);
	}
}

void DumpSymbols(IDiaEnumSymbols *pSymbols) {
	HRESULT hr;
	ULONG celt = 0;

	for (CComPtr<IDiaSymbol> pSymbol;
		SUCCEEDED(hr = pSymbols->Next(1, &pSymbol, &celt)) && celt == 1;
		pSymbol = nullptr) {
		DWORD symIndex;
		if (pSymbol->get_symIndexId(&symIndex) != S_OK) {
			Fatal("pSymbol->get_symIndexId");
		}

		DWORD symTag;
		if (pSymbol->get_symTag(&symTag) != S_OK) {
			Fatal("pSymbol->get_symTag");
		}

#if Z_DBG
		CDiaBSTR name;
		{
			if (pSymbol->get_name(&name) != S_OK || name.isNull()
				|| !name.startWith(L"z_")) {
				continue;
			}
		}
#endif

		switch (symTag) {
		default:
			break;
		case SymTagUDT:
			printf("\n%lu (%ws)\n", symIndex, SymTagStrs[symTag]);
			PrintSymbolUDT(pSymbol);
			break;
		case SymTagData:
			printf("\n%lu (%ws)\n", symIndex, SymTagStrs[symTag]);
			PrintSymbolData(pSymbol);
			break;
		}
	}
}

void PrintSymbolUDT(IDiaSymbol *pSymbol) {
	CDiaBSTR name;
	if (pSymbol->get_name(&name) == S_OK && !name.isNull()) {
		printf("struct name: %ws\n", name);
	}

	CComPtr<IDiaEnumSymbols> pChildren;
	pSymbol->findChildren(SymTagNull, nullptr, nsfCaseInsensitive, &pChildren);
	printf("{\n");
	DumpSymbols(pChildren);
	printf("}\n");
}

void PrintSymbolData(IDiaSymbol *pSymbol) {

	DWORD locationType;
	if (pSymbol->get_locationType(&locationType) != S_OK) {
		Error("get location type failed");
		return;
	}

	CDiaBSTR name;
	if (pSymbol->get_name(&name) == S_OK && !name.isNull()) {
		printf("Name: %ws\n", name);
	}

	CComPtr<IDiaSymbol> pType;
	if (pSymbol->get_type(&pType) == S_OK) {
		printf("Type: ");
		printType(pType);
		printf("\n");
	}

	printf("Location type: %ws\n", LocationTypeStrs[locationType]);
	PrintSymbolDataAddress(pSymbol, static_cast<LocationType>(locationType));
}

void PrintSymbolDataAddress(IDiaSymbol *pSym, LocationType LocTy) {
	switch (LocTy) {
	case LocIsStatic: {
		DWORD rva, addrOffset, addrSection;
		pSym->get_relativeVirtualAddress(&rva);
		pSym->get_addressOffset(&addrOffset);
		pSym->get_addressSection(&addrSection);
		printf("* RelativeVA:  %lu\n", rva);
		printf("* AddrOffset:  %lu\n", addrOffset);
		printf("* AddrSection: %lu\n", addrSection);
		break;
	}
	case LocIsRegRel:
	case LocIsThisRel: {
		LONG offset;
		pSym->get_offset(&offset);
		printf("Offset: %ld\n", offset);
		break;
	}
	default:
		break;
	}
}

