#pragma once

/*
	���ϵͳ��EPT��֧��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
CheckHvEptSupported();

/*
	��ʼ�� EPT
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
InitlizetionHvEpt();

/*
	ж�� EPT
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
WINAPI
UnInitlizetionHvEpt();

/*
	��ȡMTRR�����Ϣ
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
GetHvEptMtrrInFo();

/*
	����EPT�ڴ�
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
BuildHvEptMemory();

/*
	����EPT�ڴ�����
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
SetEptMemoryByMttrInfo(
	_In_ HvContextEntry* ContextEntry,
	_In_ INT i,
	_In_ INT j
);

/*
	����EPTP
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
SetEptp(
	_In_ HvContextEntry* ContextEntry
);

/*
	����DynamicSplit
*/
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
WINAPI
BuildHvEptDynamicSplit();

/*
	��ȡ���е�EPT_DYNAMIC_SPLIT�ڴ�
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
EPT_DYNAMIC_SPLIT*
WINAPI
GetHvEptDynamicSplit();

/*
	ˢ��EPT�ڴ�ҳ��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
WINAPI
EptUpdateTable(
	_In_ HvEptEntry* Table,
	_In_ EPT_ACCESS Access,
	_In_ ULONG64 PA,
	_In_ ULONG64 PFN
);

/*
	��ȡEPT��Ӧ��
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PEPTE
WINAPI
EptGetEpteEntry(
	_In_ HvEptEntry* Table,
	_In_ ULONG64 PA
);

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
WINAPI
EptSplitLargePage(
	_In_ EPDE_2MB* LargeEptPde,
	_In_ EPT_DYNAMIC_SPLIT* PreAllocatedBuffer
);

/*
	��ȡEPTE
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PEPTE
WINAPI
EptGetPml1Entry(
	_In_ HvEptEntry* EptPageTable,
	_In_ SIZE_T PhysicalAddress
);

/*
	��ȡEPDE
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
PEPDE_2MB
WINAPI
EptGetPml2Entry(
	_In_ HvEptEntry* EptPageTable,
	_In_ SIZE_T PhysicalAddress
);

/*
	����EPT-VMX-EXIT
*/
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
WINAPI
EptViolationHandler(
	_In_ GuestReg* Registers
);

