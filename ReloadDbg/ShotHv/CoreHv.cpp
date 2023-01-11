#include "HvPch.h"

static HvContextEntry* CoreVmxEntrys = NULL;

_Use_decl_annotations_
NTSTATUS
WINAPI
EnableIntelVT()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// ����VT������֧��
	ntStatus = CheckHvSupported();
	if (!NT_SUCCESS( ntStatus )) {
		return ntStatus;
	}

	// ����VMX����Ҫ�ı����ڴ�(Ϊ����ִ�еķǷ�ҳ�ڴ�)
	CoreVmxEntrys = (HvContextEntry*)ExAllocatePool( NonPagedPoolNx, MAX_HVCONTEXT_NUMBER * sizeof(HvContextEntry) );
	if (NULL == CoreVmxEntrys) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	RtlSecureZeroMemory(CoreVmxEntrys, MAX_HVCONTEXT_NUMBER * sizeof(HvContextEntry));

#ifdef USE_HV_EPT
	// ��ȡMtrr�����Ϣ
	ntStatus = GetHvEptMtrrInFo();
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	// ��ʼ��PAGE HOOK
	ntStatus = PHInitlizetion();
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}
#endif

	// ����Hv����Ҫ���ڴ�
	ntStatus = UtilForEachProcessor(InitlizetionIntelHvContext, NULL);
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	// Run HV ~
	ntStatus = UtilForEachProcessorDpc(DpcRunIntelHyperV, NULL);

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
DisableIntelVT()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// ж������ EPT HOOK
	PHUnAllHook();

	// �ַ�����HV
	ntStatus = UtilForEachProcessorDpc(DpcUnRunIntelHyperV, &ntStatus);

	if (CoreVmxEntrys) {
		ExFreePool(CoreVmxEntrys);
	}

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
CheckHvSupported()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	// ���Ӳ������֧��
	ntStatus = CheckHvHardwareSupported();
	if (!NT_SUCCESS( ntStatus )) {
		DBG_PRINT("Ӳ����֧�����⻯����!\r\n");
		return ntStatus;
	}

	// ���ϵͳ����֧��
	ntStatus = CheckHvOsSupported();
	if (!NT_SUCCESS( ntStatus )) {
		DBG_PRINT("ϵͳ��֧�����⻯����!\r\n");
		return ntStatus;
	}

#ifdef USE_HV_EPT
	ntStatus = CheckHvEptSupported();
	if (!NT_SUCCESS(ntStatus)) {
		DBG_PRINT("ϵͳ��֧��EPT����!\r\n");
		return ntStatus;
	}
#endif // USE_HV_EPT
	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI 
CheckHvHardwareSupported()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	CpuId Data = { 0 };
	CHAR  Vendor[0x20] = { 0 };

	/* ȡ������Ϣ - AMD����Intel */
	__cpuid( (int*)&Data, 0 );

	*(int*)(Vendor) = Data.ebx;
	*(int*)(Vendor + 4) = Data.edx;
	*(int*)(Vendor + 8) = Data.ecx;

	if (memcmp( Vendor, "GenuineIntel", 12 ) == 0) {
		ntStatus = STATUS_SUCCESS;
	}
	else if (memcmp( Vendor, "AuthenticAMD", 12 ) == 0) {
		ntStatus = STATUS_NOT_SUPPORTED;
	}
	else {
		ntStatus = STATUS_UNKNOWN_REVISION;
	}

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
CheckHvOsSupported()
{
	CpuId Data = { 0 };

	CpuidFeatureByEcx CpuidEcx = { 0 };

	Ia32FeatureControlMsr Ia32Msr = { 0 };

	__cpuid( (int*)&Data, 1 );

	CpuidEcx.all = Data.ecx;

	if ( FALSE == CpuidEcx.fields.vmx ) {
		return STATUS_HV_FEATURE_UNAVAILABLE; // ��֧�����⻯
	}

	// ��� VMXON ָ���ܷ�ִ��
	// VMXON ָ���ܷ�ִ��Ҳ���� IA32_FEATURE_CONTROL_MSR �Ĵ����Ŀ���
	// IA32_FEATURE_CONTROL_MSR[bit 0] Ϊ 0, �� VMXON ���ܵ���
	// IA32_FEATURE_CONTROL_MSR[bit 2] Ϊ 0, �� VMXON ������ SMX ����ϵͳ�����
	Ia32Msr.all = __readmsr( IA32_FEATURE_CONTROL_CODE );

	if ( (FALSE == Ia32Msr.fields.lock ) && ( FALSE == Ia32Msr.fields.enable_vmxon) ) {
		return STATUS_HV_FEATURE_UNAVAILABLE;
	}

	// �ж����⻯�����Ƿ��Ѵ�
	// ���������ʱ���Ὣ CR4.VMXE[bit 13] ��Ϊ 1
	Cr4Type Cr4 = { 0 };
	Cr4.all = __readcr4();

	if ( TRUE == Cr4.fields.vmxe )
		return STATUS_HV_OPERATION_DENIED;

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
InitlizetionIntelHvContext(
	_In_opt_ PVOID Args
)
{
	UNREFERENCED_PARAMETER(Args);

	NTSTATUS ntStatus = STATUS_SUCCESS;

	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	CoreVmxEntrys[CurrentCPU].VmxCpuNumber = CurrentCPU;

	// ��ʼ��VMX�����ڴ�
	ntStatus = InitiationVmxContext();
	if (!NT_SUCCESS(ntStatus)) {
		DBG_PRINT("��ʼ��VMX�����ڴ�ʧ��!\r\n");
		return ntStatus;
	}

#ifdef USE_HV_EPT
	// ��ʼ��EPT�����ڴ�
	ntStatus = InitlizetionHvEpt();
	if (!NT_SUCCESS(ntStatus)) {
		DBG_PRINT("����EPTʧ��!\r\n");
	}
#endif
	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
InitiationVmxContext()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress = ExAllocatePool(NonPagedPoolNx, PAGE_SIZE);
	if (NULL == CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress) {
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress = ExAllocatePool(NonPagedPoolNx, PAGE_SIZE);
	if (NULL == CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress) {
		ClearVmxContext();
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress = ExAllocatePool(NonPagedPoolNx, PAGE_SIZE);
	if (NULL == CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress) {
		ClearVmxContext();
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress = ExAllocatePool(NonPagedPoolNx, PAGE_SIZE * 3);
	if (NULL == CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress) {
		ClearVmxContext();
		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	RtlSecureZeroMemory(CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress, PAGE_SIZE);
	RtlSecureZeroMemory(CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress, PAGE_SIZE);
	RtlSecureZeroMemory(CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress, PAGE_SIZE);
	RtlSecureZeroMemory(CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress, PAGE_SIZE * 3);

	CoreVmxEntrys[CurrentCPU].VmxOnRegionPhyAddress = MmGetPhysicalAddress(CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress).QuadPart;
	CoreVmxEntrys[CurrentCPU].VmxCsRegionPhyAddress = MmGetPhysicalAddress(CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress).QuadPart;
	CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionPhyAddress = MmGetPhysicalAddress(CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress).QuadPart;

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
RunIntelHyperV()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	// ���� Guest Rip\Rsp
	if (FALSE == CoreVmxEntrys[CurrentCPU].VmxOnOFF)
	{
		AsmStackPointer( &CoreVmxEntrys[CurrentCPU].VmxGuestRsp );
		AsmNextInstructionPointer( &CoreVmxEntrys[CurrentCPU].VmxGuestRip );

		if (TRUE == CoreVmxEntrys[CurrentCPU].VmxOnOFF) // Guest(VMM) ����½��
			return STATUS_SUCCESS;
	}

	// ���� VMCS
	ntStatus = InitiationVmCsContext();
	if (!NT_SUCCESS(ntStatus)) {

		DBG_PRINT("���⻯����VMCS����ʧ��!\r\n");
		return ntStatus;
	}

	// ���� Intel VT
	ntStatus = ExecuteVmlaunch();
	if (!NT_SUCCESS(ntStatus)) {
		DBG_PRINT("���� Intel VTʧ��!\r\n");
		return ntStatus;
	}

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
UnRunIntelHyperV()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	if (CoreVmxEntrys[CurrentCPU].VmxOnOFF)
	{
		// �˳� VT
		HvVmCall(CallExitVt);
		
		// ��� VMX ����
		ClearVmxContext();

		// ж�� EPT
		UnInitlizetionHvEpt();
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
InitiationVmCsContext()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	if (TRUE == CoreVmxEntrys[CurrentCPU].VmxOnOFF)
	{
		return STATUS_SUCCESS;
	}

	// ��ʼ�� Guest �� Host ����
	// (1). ���� Guest ״̬
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cs = __readcs();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.ds = __readds();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.ss = __readss();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.es = __reades();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.fs = __readfs();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.gs = __readgs();

	CoreVmxEntrys[CurrentCPU].VmxGuestState.ldtr = __sldt();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.tr = __str();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.rflags = __readeflags();

	CoreVmxEntrys[CurrentCPU].VmxGuestState.rsp = CoreVmxEntrys[CurrentCPU].VmxGuestRsp; // ���� GUEST �� RSP��RIP
	CoreVmxEntrys[CurrentCPU].VmxGuestState.rip = CoreVmxEntrys[CurrentCPU].VmxGuestRip;

	__sgdt(&(CoreVmxEntrys[CurrentCPU].VmxGuestState.gdt));
	__sidt(&(CoreVmxEntrys[CurrentCPU].VmxGuestState.idt));

	Cr0Type cr0_mask = { 0 };
	Cr0Type cr0_shadow = { __readcr0() };

	Cr4Type cr4_mask = { 0 };
	Cr4Type cr4_shadow = { __readcr4() };

	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr3 = __readcr3();
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0 = ((__readcr0() & __readmsr(IA32_VMX_CR0_FIXED1)) | __readmsr(IA32_VMX_CR0_FIXED0));
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4 = ((__readcr4() & __readmsr(IA32_VMX_CR4_FIXED1)) | __readmsr(IA32_VMX_CR4_FIXED0));

	cr0_shadow.all = CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0;
	cr4_shadow.all = CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4;

	cr4_mask.fields.vmxe = true;
	cr4_shadow.fields.vmxe = false;

	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0_mask = cr0_mask.all;
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0_shadow = cr0_shadow.all;
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4_mask = cr4_mask.all;
	CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4_shadow = cr4_shadow.all;

	CoreVmxEntrys[CurrentCPU].VmxGuestState.dr7 = __readdr(7);
	CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_debugctl = __readmsr(IA32_DEBUGCTL);
	CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_sysenter_cs = __readmsr(IA32_SYSENTER_CS);
	CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_sysenter_eip = __readmsr(IA32_SYSENTER_EIP);
	CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_sysenter_esp = __readmsr(IA32_SYSENTER_ESP);

	__writecr0(CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0);
	__writecr4(CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4);

	CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_efer = __readmsr(MSR_IA32_EFER); // ��� Guest EFER

	// (2). ��ʼ�� Host ״̬
	CoreVmxEntrys[CurrentCPU].VmxHostState.cr0 = __readcr0();
	CoreVmxEntrys[CurrentCPU].VmxHostState.cr3 = __readcr3();
	CoreVmxEntrys[CurrentCPU].VmxHostState.cr4 = __readcr4();

	CoreVmxEntrys[CurrentCPU].VmxHostState.cs = __readcs() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.ds = __readds() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.ss = __readss() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.es = __reades() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.fs = __readfs() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.gs = __readgs() & 0xF8;
	CoreVmxEntrys[CurrentCPU].VmxHostState.tr = __str();

	// (3). ���� HOST �� RSP��RIP
	CoreVmxEntrys[CurrentCPU].VmxHostState.rsp = ROUNDUP(((ULONG64)CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress + 0x2000), PAGE_SIZE);
	CoreVmxEntrys[CurrentCPU].VmxHostState.rip = (ULONG_PTR)AsmCallVmxExitHandler;

	CoreVmxEntrys[CurrentCPU].VmxHostState.msr_sysenter_cs = __readmsr(IA32_SYSENTER_CS);
	CoreVmxEntrys[CurrentCPU].VmxHostState.msr_sysenter_eip = __readmsr(IA32_SYSENTER_EIP);
	CoreVmxEntrys[CurrentCPU].VmxHostState.msr_sysenter_esp = __readmsr(IA32_SYSENTER_ESP);

	CoreVmxEntrys[CurrentCPU].VmxHostState.msr_efer = __readmsr(MSR_IA32_EFER); // ��� Host EFER

	__sgdt(&(CoreVmxEntrys[CurrentCPU].VmxHostState.gdt));
	__sidt(&(CoreVmxEntrys[CurrentCPU].VmxHostState.idt));

	// Setup Msr Context
	auto ntStatus = InitiationMsrContext();
	if (!NT_SUCCESS(ntStatus)) {
		return ntStatus;
	}

	// ��д VMCS �е� GUEST ״̬
	// (1). CS\SS\DS\ES\FS\GS\TR �Ĵ���
	ULONG_PTR uBase, uLimit, uAccess; uBase = uLimit = uAccess = 0;
	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.cs, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_CS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.cs);
	VmxCsWrite(GUEST_CS_LIMIT, uLimit);
	VmxCsWrite(GUEST_CS_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_CS_BASE, uBase);

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.ss, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_SS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.ss);
	VmxCsWrite(GUEST_SS_LIMIT, uLimit);
	VmxCsWrite(GUEST_SS_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_SS_BASE, uBase);

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.ds, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_DS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.ds);
	VmxCsWrite(GUEST_DS_LIMIT, uLimit);
	VmxCsWrite(GUEST_DS_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_DS_BASE, uBase);

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.es, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_ES_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.es);
	VmxCsWrite(GUEST_ES_LIMIT, uLimit);
	VmxCsWrite(GUEST_ES_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_ES_BASE, uBase);
	VmxCsWrite(HOST_ES_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.es);

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.fs, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_FS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.fs);
	VmxCsWrite(GUEST_FS_LIMIT, uLimit);
	VmxCsWrite(GUEST_FS_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_FS_BASE, uBase);
	CoreVmxEntrys[CurrentCPU].VmxHostState.fsbase = uBase;

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.gs, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_GS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.gs);
	uBase = __readmsr(MSR_GS_BASE);
	VmxCsWrite(GUEST_GS_LIMIT, uLimit);
	VmxCsWrite(GUEST_GS_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_GS_BASE, uBase);
	CoreVmxEntrys[CurrentCPU].VmxHostState.gsbase = uBase;

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.tr, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_TR_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.tr);
	VmxCsWrite(GUEST_TR_LIMIT, uLimit);
	VmxCsWrite(GUEST_TR_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_TR_BASE, uBase);
	CoreVmxEntrys[CurrentCPU].VmxHostState.trbase = uBase;

	UtilGetSelectorInfoBySelector(CoreVmxEntrys[CurrentCPU].VmxGuestState.ldtr, &uBase, &uLimit, &uAccess);
	VmxCsWrite(GUEST_LDTR_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxGuestState.ldtr);
	VmxCsWrite(GUEST_LDTR_LIMIT, uLimit);
	VmxCsWrite(GUEST_LDTR_AR_BYTES, uAccess);
	VmxCsWrite(GUEST_LDTR_BASE, uBase);

	// (2). GDTR\IDTR ��Ϣ
	VmxCsWrite(GUEST_GDTR_BASE, CoreVmxEntrys[CurrentCPU].VmxGuestState.gdt.uBase);
	VmxCsWrite(GUEST_GDTR_LIMIT, CoreVmxEntrys[CurrentCPU].VmxGuestState.gdt.uLimit);

	VmxCsWrite(GUEST_IDTR_BASE, CoreVmxEntrys[CurrentCPU].VmxGuestState.idt.uBase);
	VmxCsWrite(GUEST_IDTR_LIMIT, CoreVmxEntrys[CurrentCPU].VmxGuestState.idt.uLimit);

	// (3). ���ƼĴ��� CR0\CR3\CR4
	VmxCsWrite(CR4_GUEST_HOST_MASK, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4_mask);
	VmxCsWrite(CR0_GUEST_HOST_MASK, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0_mask);
	VmxCsWrite(CR4_READ_SHADOW, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4_shadow);
	VmxCsWrite(CR0_READ_SHADOW, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0_shadow);

	VmxCsWrite(GUEST_CR3, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr3);
	VmxCsWrite(GUEST_CR0, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr0);
	VmxCsWrite(GUEST_CR4, CoreVmxEntrys[CurrentCPU].VmxGuestState.cr4);

	// (4). RSP\RIP �� RFLAGS��DR7
	VmxCsWrite(GUEST_IA32_DEBUGCTL, CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_debugctl);
	VmxCsWrite(GUEST_DR7, CoreVmxEntrys[CurrentCPU].VmxGuestState.dr7);
	VmxCsWrite(GUEST_RSP, CoreVmxEntrys[CurrentCPU].VmxGuestState.rsp);
	VmxCsWrite(GUEST_RIP, CoreVmxEntrys[CurrentCPU].VmxGuestState.rip);
	VmxCsWrite(GUEST_RFLAGS, CoreVmxEntrys[CurrentCPU].VmxGuestState.rflags);

	VmxCsWrite(GUEST_EFER, CoreVmxEntrys[CurrentCPU].VmxGuestState.msr_efer);

	// ��ʼ��������(HOST)״̬
	VmxCsWrite(HOST_CS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.cs);
	VmxCsWrite(HOST_SS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.ss);
	VmxCsWrite(HOST_DS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.ds);

	VmxCsWrite(HOST_FS_BASE, CoreVmxEntrys[CurrentCPU].VmxHostState.fsbase);
	VmxCsWrite(HOST_FS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.fs);

	VmxCsWrite(HOST_GS_BASE, CoreVmxEntrys[CurrentCPU].VmxHostState.gsbase);
	VmxCsWrite(HOST_GS_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.gs);

	VmxCsWrite(HOST_TR_BASE, CoreVmxEntrys[CurrentCPU].VmxHostState.trbase);
	VmxCsWrite(HOST_TR_SELECTOR, CoreVmxEntrys[CurrentCPU].VmxHostState.tr);

	VmxCsWrite(HOST_GDTR_BASE, CoreVmxEntrys[CurrentCPU].VmxHostState.gdt.uBase);
	VmxCsWrite(HOST_IDTR_BASE, CoreVmxEntrys[CurrentCPU].VmxHostState.idt.uBase);

	VmxCsWrite(HOST_CR0, CoreVmxEntrys[CurrentCPU].VmxHostState.cr0);
	VmxCsWrite(HOST_CR4, CoreVmxEntrys[CurrentCPU].VmxHostState.cr4);
	VmxCsWrite(HOST_CR3, CoreVmxEntrys[CurrentCPU].VmxHostState.cr3);

	VmxCsWrite(HOST_RIP, CoreVmxEntrys[CurrentCPU].VmxHostState.rip);
	VmxCsWrite(HOST_RSP, CoreVmxEntrys[CurrentCPU].VmxHostState.rsp);

	VmxCsWrite(HOST_EFER, CoreVmxEntrys[CurrentCPU].VmxHostState.msr_efer);

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
InitiationMsrContext()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	// ִ�� vmxon/vmclear/vmptrld ��ʼ�������� VMCS ����
	auto ntStatus = ExecuteVmxOn();

	if (!NT_SUCCESS(ntStatus)) {
		DBG_PRINT("ִ�� vmxon/vmclear/vmptrld ʧ��!\r\n");
		return ntStatus;
	}

	// 1. ���û���pin��vmִ�п�����Ϣ�� ��Pin-Based VM-Execution Controls��
	Ia32VmxBasicMsr ia32basicMsr = { 0 };
	ia32basicMsr.all = __readmsr(MSR_IA32_VMX_BASIC);

	VmxPinBasedControls vm_pinctl_requested = { 0 }; // �������� Pin-Based VM-Execution Controls
		// �μ���Ƥ�� (A-2 Vol. 3D)�������������⻯������(��2.5��)
		// ���IA32_VMX_BASIC MSR�е�λ55����ȡΪ1����ʹ�� MSR_IA32_VMX_TRUE_PINBASED_CTLS 
	VmxPinBasedControls vm_pinctl = {
		VmxAdjustControlValue((ia32basicMsr.fields.vmx_capability_hint) ? MSR_IA32_VMX_TRUE_PINBASED_CTLS : MSR_IA32_VMX_PINBASED_CTLS,
								vm_pinctl_requested.all) };	// ���� Pin-Based VM-Execution Controls ��ֵ
	ntStatus = VmxCsWrite(PIN_BASED_VM_EXEC_CONTROL, vm_pinctl.all);	// ���� Pin-Based VM-Execution Controls

	// 2. ���û��ڴ���������vmִ�п�����Ϣ�� ��Primary Processor-Based VM-Execution Controls��
	VmxProcessorBasedControls vm_procctl_requested = { 0 }; // �������� Primary Processor-Based VM-Execution Controls
	//vm_procctl_requested.fields.cr3_load_exiting  = TRUE;	// ����д��Cr3
	//vm_procctl_requested.fields.cr3_store_exiting = TRUE;	// ���ض�ȡCr3
	vm_procctl_requested.fields.use_msr_bitmaps = TRUE;		// ����MSR bitmap, ����msr�Ĵ�������,��������,��Ȼ�κη�msr�Ĳ������ᵼ��vmexit
	vm_procctl_requested.fields.activate_secondary_control = TRUE; // ������չ�ֶ�
	VmxProcessorBasedControls vm_procctl = {
		VmxAdjustControlValue((ia32basicMsr.fields.vmx_capability_hint) ? MSR_IA32_VMX_TRUE_PROCBASED_CTLS : MSR_IA32_VMX_PROCBASED_CTLS,
								vm_procctl_requested.all) }; // ���� Primary Processor-Based VM-Execution Controls ��ֵ
	ntStatus = VmxCsWrite(CPU_BASED_VM_EXEC_CONTROL, vm_procctl.all);

	// 3. ���û��ڴ������ĸ���vmִ�п�����Ϣ�����չ�ֶ� ��Secondary Processor-Based VM-Execution Controls��
	VmxSecondaryProcessorBasedControls vm_procctl2_requested = { 0 };
	vm_procctl2_requested.fields.enable_rdtscp = TRUE;			 // for Win10
	vm_procctl2_requested.fields.enable_invpcid = TRUE;			 // for Win10
	vm_procctl2_requested.fields.enable_xsaves_xstors = TRUE;	 // for Win10

#ifdef USE_HV_EPT
	// ���￴�Ƿ���Ҫ���� EPT
	{
		vm_procctl2_requested.fields.enable_ept = TRUE;  // ���� EPT
		vm_procctl2_requested.fields.enable_vpid = TRUE; // ���� VPID
												// (VPID��������TLB��������, ����VPID�ο���ϵͳ���⻯:ԭ����ʵ�֡�(��139ҳ)����Ƥ��(Vol. 3C 28-1))
	}
#endif

	VmxSecondaryProcessorBasedControls vm_procctl2 = { VmxAdjustControlValue(
			MSR_IA32_VMX_PROCBASED_CTLS2, vm_procctl2_requested.all) };
	ntStatus = VmxCsWrite(SECONDARY_VM_EXEC_CONTROL, vm_procctl2.all);

	// 4. ����vm-entry������
	VmxVmentryControls vm_entryctl_requested = { 0 };
	vm_entryctl_requested.fields.ia32e_mode_guest = TRUE; // 64ϵͳ������, �ο������������⻯������(��212ҳ)
	VmxVmentryControls vm_entryctl = { VmxAdjustControlValue(
		(ia32basicMsr.fields.vmx_capability_hint) ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS,
		vm_entryctl_requested.all) };
	ntStatus = VmxCsWrite(VM_ENTRY_CONTROLS, vm_entryctl.all);

	// 5. ����vm-exit������Ϣ��
	VmxmexitControls vm_exitctl_requested = { 0 };
	vm_exitctl_requested.fields.acknowledge_interrupt_on_exit = TRUE;
	vm_exitctl_requested.fields.host_address_space_size = TRUE; // 64ϵͳ������, �ο������������⻯������(��219ҳ)
	VmxmexitControls vm_exitctl = { VmxAdjustControlValue(
		(ia32basicMsr.fields.vmx_capability_hint) ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS,
		vm_exitctl_requested.all) };
	ntStatus = VmxCsWrite(VM_EXIT_CONTROLS, vm_exitctl.all);

	// ��д MsrBitMap
	ntStatus = VmxCsWrite(MSR_BITMAP, CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionPhyAddress);    // λͼ

	// ��д ExceptionBitMap
	ULONG_PTR exception_bitmap = 0;
	//exception_bitmap |= (1 << InterruptionVector::EXCEPTION_VECTOR_PAGE_FAULT); // �������� #PF �쳣(���� VM-exit)
	VmxCsWrite(EXCEPTION_BITMAP, exception_bitmap);

	// VMCS link pointer �ֶγ�ʼ��
	ULONG_PTR link_pointer = 0xFFFFFFFFFFFFFFFFL;
	ntStatus = VmxCsWrite(VMCS_LINK_POINTER, link_pointer);

#ifdef USE_HV_EPT
	{
		VmxCsWrite(EPT_POINTER, CoreVmxEntrys[CurrentCPU].VmxEptp.Flags);	// ��д EptPointer	
		ULONG processor = KeGetCurrentProcessorNumberEx(NULL);
		VmxCsWrite(VIRTUAL_PROCESSOR_ID, (uintptr_t)processor + 1);
	}
#endif

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
ExecuteVmxOn()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	ULONG_PTR uVmxBasic = __readmsr(IA32_VMX_BASIC_MSR_CODE); // ��ȡ VMID

	// 1. ���汾�� VMID
	*(PULONG32)CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress = (ULONG32)uVmxBasic;
	*(PULONG32)CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress = (ULONG32)uVmxBasic;

	// 2. ����CR4
	Cr4Type cr4 = { 0 };
	cr4.all = __readcr4();
	cr4.fields.vmxe = true; // CR4.VMXE ��Ϊ 1, ���� VMX ָ��
	__writecr4(cr4.all);

	// 4. ִ�� VMON ָ��
	// VMXON ��ָ�����:
	// 1). VMXON ָ���Ƿ񳬹������ַ���, �����Ƿ� 4K ����
	// 2). VMXON �����ͷ DWORD ֵ�Ƿ���� VMCS ID
	// 3). ���Ὣ�������ַ��Ϊָ�룬ָ�� VMXON ����
	// 4). ���տ�ͨ�� eflags.cf �Ƿ�Ϊ 0 ���ж�ִ�гɹ�
	__vmx_on(&CoreVmxEntrys[CurrentCPU].VmxOnRegionPhyAddress);

	FlagReg elf = { 0 };
	elf.all = __readeflags();

	if (elf.fields.cf != 0)
	{
		DBG_PRINT("Cpu:[%d] vmxon ����ʧ��!\n", CoreVmxEntrys[CurrentCPU].VmxCpuNumber);
		ntStatus = STATUS_UNSUCCESSFUL;
		goto DONE;
	}

	CoreVmxEntrys[CurrentCPU].VmxOnOFF = true; // ���� VT ���⻯��־

	// 5. ִ�� VMCLEAR ָ������ʼ�� VMCS ���ƿ���ض���Ϣ
	if (__vmx_vmclear(&CoreVmxEntrys[CurrentCPU].VmxCsRegionPhyAddress) != 0) {
		DBG_PRINT("Cpu:[%d] vmclear ����ʧ��!\n", CoreVmxEntrys[CurrentCPU].VmxCpuNumber);
		return STATUS_UNSUCCESSFUL;
	}
	if (__vmx_vmptrld(&CoreVmxEntrys[CurrentCPU].VmxCsRegionPhyAddress)) {
		DBG_PRINT("Cpu:[%d] vmptrld ����ʧ��!\n", CoreVmxEntrys[CurrentCPU].VmxCpuNumber);
		ntStatus = STATUS_UNSUCCESSFUL;
		goto DONE;
	}

DONE:
	if (!NT_SUCCESS(ntStatus)) {
		// ��ֹHV��ʼ��
		StopHvInitlizetion();
	}

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS 
WINAPI
ExecuteVmlaunch()
{
	NTSTATUS ntStatus = STATUS_SUCCESS;

	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	__vmx_vmlaunch(); // �����仰ִ�гɹ�,�Ͳ��᷵��

	if (CoreVmxEntrys[CurrentCPU].VmxOnOFF)	 // ���˴����� VT ����ʧ��
	{
		size_t uError = 0;
		uError = VmxCsRead(VM_INSTRUCTION_ERROR);	 // �ο���Ƥ�� (Vol. 3C 30-29)
		DBG_PRINT("Cpu:[%d] ErrorCode:[%x] vmlaunch ָ�����ʧ��!\r\n", CoreVmxEntrys[CurrentCPU].VmxCpuNumber, (ULONG)uError);

		__vmx_off(); // �ر� CPU �� VMX ģʽ

		// ��ֹHV��ʼ��
		StopHvInitlizetion();

		ntStatus = STATUS_UNSUCCESSFUL;
	}

	DBG_PRINT("Cpu:[%d] VT ���⻯ʧ��!\r\n", CurrentCPU);

	__writeds(0x28 | 0x3); // RTL��Ϊ1
	__writees(0x28 | 0x3);
	__writefs(0x50 | 0x3);

	return ntStatus;
}

_Use_decl_annotations_
NTSTATUS
WINAPI
ClearVmxContext()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	if (CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress)
	{
		ExFreePool(CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress);
		CoreVmxEntrys[CurrentCPU].VmxOnRegionLinerAddress = nullptr;
	}

	if (CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress)
	{
		ExFreePool(CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress);
		CoreVmxEntrys[CurrentCPU].VmxCsRegionLinerAddress = nullptr;
	}

	if (CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress)
	{
		ExFreePool(CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress);
		CoreVmxEntrys[CurrentCPU].VmxMsrBitMapRegionLinerAddress = nullptr;
	}

	if (CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress)
	{
		ExFreePool((PVOID)CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress);
		CoreVmxEntrys[CurrentCPU].VmxStackRootRegionLinerAddress = nullptr;
	}

	return STATUS_SUCCESS;
}

_Use_decl_annotations_
VOID 
WINAPI 
StopHvInitlizetion()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();

	Cr4Type cr4 = { 0 };
	cr4.all = __readcr4();
	cr4.fields.vmxe = false;
	__writecr4(cr4.all);

	CoreVmxEntrys[CurrentCPU].VmxOnOFF = false;

	// ��� VMX ����
	ClearVmxContext();
}

_Use_decl_annotations_
BOOLEAN
WINAPI
VmxCsWrite(
	_In_ ULONG64 target_field,
	_In_ ULONG64 value
)
{
	ULONG_PTR dwField = target_field;

	if (__vmx_vmwrite(dwField, value) != 0)
	{
		return FALSE;
	}

	return TRUE;
}

_Use_decl_annotations_
ULONG64 
WINAPI
VmxCsRead(
	_In_ ULONG64 target_field
)
{
	ULONG64 dwValue = 0xFFFFFFFFFFFFFFFF;
	ULONG64 dwField = target_field;

	if (__vmx_vmread(dwField, &dwValue) != 0)
	{
		return dwValue;
	}

	return dwValue;
}

ULONG
WINAPI
VmxAdjustControlValue(
	_In_ ULONG Msr,
	_In_ ULONG Ctl
)
{
	// �ο��ԡ����������⻯������(2.5.6.3)
	LARGE_INTEGER MsrValue = { 0 };
	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;     //ǰ32λΪ0��λ�ñ�ʾ��Щ��������λ0
	Ctl |= MsrValue.LowPart;      //��32λΪ1��λ�ñ�ʾ��Щ��������λ1
	return Ctl;
}

HvContextEntry* 
WINAPI
GetHvContextEntry()
{
	ULONG CurrentCPU = KeGetCurrentProcessorNumber();
	return &CoreVmxEntrys[CurrentCPU];
}

_Use_decl_annotations_
VOID
WINAPI
HvRestoreRegisters()
{
	ULONG_PTR GuestCr3 = 0;
	ULONG_PTR FsBase = 0, GsBase = 0;
	ULONG_PTR GdtrBase = 0, GdtrLimit = 0;
	ULONG_PTR IdtrBase = 0, IdtrLimit = 0;

	// ��ԭCr3
	__vmx_vmread(GUEST_CR3, &GuestCr3);
	__writecr3(GuestCr3);

	// ��ԭ FS Base
	__vmx_vmread(GUEST_FS_BASE, &FsBase);
	__writemsr(IA32_FS_BASE, FsBase);

	// ��ԭ Gs Base
	__vmx_vmread(GUEST_GS_BASE, &GsBase);
	__writemsr(MSR_GS_BASE, GsBase);

	// ��ԭ GDTR
	__vmx_vmread(GUEST_GDTR_BASE, &GdtrBase);
	__vmx_vmread(GUEST_GDTR_LIMIT, &GdtrLimit);
	AsmReloadGdtr((void*)GdtrBase, (ULONG)GdtrLimit);

	// ��ԭ IDTR
	__vmx_vmread(GUEST_IDTR_BASE, &IdtrBase);
	__vmx_vmread(GUEST_IDTR_LIMIT, &IdtrLimit);
	AsmReloadIdtr((void*)IdtrBase, (ULONG)IdtrLimit);
}

_Use_decl_annotations_
BOOLEAN
WINAPI
HvVmCall(
	_In_ ULONG CallNumber,	/*���*/
	_In_ ULONG64 arg1,
	_In_ ULONG64 arg2,
	_In_ ULONG64 arg3
)
{
	// ��֤ HV �Ƿ���
	CpuId Data = { 0 };
	
	__cpuid( (int*)&Data, CupidHvCheck );
	
	if ('SBTX' != Data.eax) {
		return FALSE;
	}
	
	// vmxcall
	AsmVmxCall(CallNumber, arg1, arg2, arg3);

	return TRUE;
}