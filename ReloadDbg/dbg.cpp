#include"dbg.h"
#include"HookFunc.h"


extern SYMBOLS_DATA g_SymbolsData;
extern __DbgkpWakeTarget DbgkpWakeTarget;
extern __DbgkpSuppressDbgMsg DbgkpSuppressDbgMsg;
extern __DbgkpMarkProcessPeb DbgkpMarkProcessPeb;
extern __DbgkpSendApiMessage DbgkpSendApiMessage;
extern __DbgkCreateThread OriginalDbgkCreateThread;
extern __DbgkpSendErrorMessage DbgkpSendErrorMessage;
extern  __NtTerminateProcess OrignalNtTerminateProcess;
extern __DbgkpSendApiMessageLpc DbgkpSendApiMessageLpc;
extern __PsCaptureExceptionPort PsCaptureExceptionPort;
extern __PsGetNextProcessThread  PsGetNextProcessThread;
extern __KiDispatchException OrignalKiDispatchException;
extern __NtCreateUserProcess OrignalNtCreateUserProcess;
extern __DbgkpSectionToFileHandle DbgkpSectionToFileHandle;
extern __DbgkSendSystemDllMessages DbgkSendSystemDllMessages;
extern __DbgkpPostFakeThreadMessages  DbgkpPostFakeThreadMessages;
extern __DbgkpPostFakeProcessCreateMessages DbgkpPostFakeProcessCreateMessages;


extern KSPIN_LOCK g_DebugLock;
extern DebugInfomation g_Debuginfo;


PVOID 	OriginalDbgkpQueueMessage = NULL;
PVOID 	OriginalNtCreateDebugObject = NULL;
PVOID 	OriginalDbgkForwardException = NULL;
PVOID   OriginalNtDebugActiveProcess = NULL;
PVOID   OriginalDbgkMapViewOfSection = NULL;
PVOID 	OriginalDbgkUnMapViewOfSection = NULL;
PVOID 	OriginalDbgkpSetProcessDebugObject = NULL;

POBJECT_TYPE* g_DbgkDebugObjectType;
BOOLEAN HookDbgkDebugObjectType()
{
	ULONG64 addr = 0;
	ULONG templong = 0;
	UNICODE_STRING ObjectTypeName;

	g_DbgkDebugObjectType = (POBJECT_TYPE*)g_SymbolsData.DbgkDebugObjectType;
	if (g_DbgkDebugObjectType == 0)
		return FALSE;

	RtlInitUnicodeString(&ObjectTypeName, L"YCData");
	OBJECT_TYPE_INITIALIZER_WIN10 ObjectTypeInitializer;
	POBJECT_TYPE* DbgkDebugObjectType = g_DbgkDebugObjectType;
	memcpy(&ObjectTypeInitializer, &(*DbgkDebugObjectType)->TypeInfo, sizeof(OBJECT_TYPE_INITIALIZER_WIN10));

	//����ָ�����Ȩ��
	ObjectTypeInitializer.DeleteProcedure = NULL;
	ObjectTypeInitializer.CloseProcedure = NULL;
	ObjectTypeInitializer.GenericMapping.GenericRead = 0x00020001;
	ObjectTypeInitializer.GenericMapping.GenericWrite = 0x00020002;
	ObjectTypeInitializer.GenericMapping.GenericExecute = 0x00120000;
	ObjectTypeInitializer.GenericMapping.GenericAll = 0x001f000f;
	ObjectTypeInitializer.ValidAccessMask = 0x001f000f;

	NTSTATUS status = ObCreateObjectType(&ObjectTypeName, &ObjectTypeInitializer, NULL, (PVOID*)g_DbgkDebugObjectType);

	if (!NT_SUCCESS(status))
	{
		if (status == STATUS_OBJECT_NAME_COLLISION)
		{
			POBJECT_TYPE* ObTypeIndexTable = (POBJECT_TYPE*)g_SymbolsData.ObTypeIndexTable;
			if (!ObTypeIndexTable)
				return FALSE;

			ULONG Index = 2;
			while (ObTypeIndexTable[Index])
			{
				if (&ObTypeIndexTable[Index]->Name)
				{
					if (ObTypeIndexTable[Index]->Name.Buffer)
					{
						if (RtlCompareUnicodeString(&ObTypeIndexTable[Index]->Name, &ObjectTypeName, FALSE) == 0)
						{
							*g_DbgkDebugObjectType = ObTypeIndexTable[Index];
							return TRUE;
						}
					}
				}

				Index++;
			}
		}
	}

	return TRUE;
}

/*
	win7-win10��
	���õ�debugport�ĺ���
	PsGetProcessDebugPort       //��ȡdebugport��ֵ��������
	DbgkpSetProcessDebugObject  //���ﲻ��debugport��ֵд��eprocess��debugport�ֶΣ�Ҳ������DbgkpMarkProcessPeb
	DbgkpMarkProcessPeb         //DbgkClearProcessDebugObject��DbgkpCloseObject��objectType��CloseProcedure������ֱ�Ӳ�ʵ�֣���DbgkpSetProcessDebugObject�л����
	DbgkCreateThread            //��ʵ���ڲ��е㳤,��ʵ���̻߳ص�
	PspExitThread         	    //��ʵ�֣���Ҫ�߳��˳���Ϣ
	DbgkExitThread      		//PspExitThread�����DbgkExitThread�����涼��ʵ��
	DbgkpQueueMessage			//ʵ�ֱȽϼ�
	KiDispatchException   		//���Բ�ʵ�֣����ں˵��������Ȳ����쳣��Ҫgn���������ں˵���,���Ҳ����������int 2d
	DbgkForwardException		//����������ԭ����
	NtQueryInformationProcess 	//������
	DbgkClearProcessDebugObject //��ʵ�� 
	DbgkpCloseObject            //��ʵ��  
	DbgkMapViewOfSection		//����������ԭ����
	DbgkUnMapViewOfSection		//����������ԭ����
	DbgkExitProcess				//��ʵ��
*/

BOOLEAN DbgInit()
{
	//��ʼ������	
	DbgkpWakeTarget = (__DbgkpWakeTarget)g_SymbolsData.DbgkpWakeTarget;
	DbgkpSuppressDbgMsg = (__DbgkpSuppressDbgMsg)g_SymbolsData.DbgkpSuppressDbgMsg;
	DbgkpSendApiMessage = (__DbgkpSendApiMessage)g_SymbolsData.DbgkpSendApiMessage;
	DbgkpMarkProcessPeb = (__DbgkpMarkProcessPeb)g_SymbolsData.DbgkpMarkProcessPeb;
	DbgkpSendErrorMessage = (__DbgkpSendErrorMessage)g_SymbolsData.DbgkpSendErrorMessage;
	PsGetNextProcessThread = (__PsGetNextProcessThread)g_SymbolsData.PsGetNextProcessThread;
	DbgkpSendApiMessageLpc = (__DbgkpSendApiMessageLpc)g_SymbolsData.DbgkpSendApiMessageLpc;
	PsCaptureExceptionPort = (__PsCaptureExceptionPort)g_SymbolsData.PsCaptureExceptionPort;
	DbgkpSectionToFileHandle = (__DbgkpSectionToFileHandle)g_SymbolsData.DbgkpSectionToFileHandle;
	DbgkSendSystemDllMessages = (__DbgkSendSystemDllMessages)g_SymbolsData.DbgkSendSystemDllMessages;
	DbgkpPostFakeThreadMessages = (__DbgkpPostFakeThreadMessages)g_SymbolsData.DbgkpPostFakeThreadMessages;
	DbgkpPostFakeProcessCreateMessages = (__DbgkpPostFakeProcessCreateMessages)g_SymbolsData.DbgkpPostFakeProcessCreateMessages;
	
	//��ʼ�����Զ�����������
	InitializeListHead(&g_Debuginfo.List);
	KeInitializeSpinLock(&g_DebugLock);

	//���￪ʼhook����
#ifdef WINVM
	PHHook(g_SymbolsData.DbgkCreateThread, DbgkCreateThread, (PVOID*)&OriginalDbgkCreateThread);
	PHHook(g_SymbolsData.DbgkpQueueMessage, DbgkpQueueMessage, (PVOID*)&OriginalDbgkpQueueMessage);
	PHHook(g_SymbolsData.NtTerminateProcess, NtTerminateProcess, (PVOID*)&OrignalNtTerminateProcess);
	PHHook(g_SymbolsData.NtCreateUserProcess, NtCreateUserProcess, (PVOID*)&OrignalNtCreateUserProcess);
	PHHook(g_SymbolsData.KiDispatchException, KiDispatchException, (PVOID*)&OrignalKiDispatchException);
	PHHook(g_SymbolsData.NtCreateDebugObject, NtCreateDebugObject, (PVOID*)&OriginalNtCreateDebugObject);
	PHHook(g_SymbolsData.NtDebugActiveProcess, NtDebugActiveProcess, (PVOID*)&OriginalNtDebugActiveProcess);
	PHHook(g_SymbolsData.DbgkForwardException, DbgkForwardException, (PVOID*)&OriginalDbgkForwardException);
	PHHook(g_SymbolsData.DbgkMapViewOfSection, DbgkMapViewOfSection, (PVOID*)&OriginalDbgkMapViewOfSection);
	PHHook(g_SymbolsData.DbgkUnMapViewOfSection, DbgkUnMapViewOfSection, (PVOID*)&OriginalDbgkUnMapViewOfSection);
	PHHook(g_SymbolsData.DbgkpSetProcessDebugObject, DbgkpSetProcessDebugObject, (PVOID*)&OriginalDbgkpSetProcessDebugObject);
	PHActivateHooks();
#else
	hook_function(g_SymbolsData.DbgkCreateThread, DbgkCreateThread, (PVOID*)&OriginalDbgkCreateThread);
	hook_function(g_SymbolsData.DbgkpQueueMessage, DbgkpQueueMessage, (PVOID*)&OriginalDbgkpQueueMessage);
	hook_function(g_SymbolsData.NtTerminateProcess, NtTerminateProcess, (PVOID*)&OrignalNtTerminateProcess);
	hook_function(g_SymbolsData.NtCreateUserProcess, NtCreateUserProcess, (PVOID*)&OrignalNtCreateUserProcess);
	hook_function(g_SymbolsData.KiDispatchException, KiDispatchException, (PVOID*)&OrignalKiDispatchException);
	hook_function(g_SymbolsData.NtCreateDebugObject, NtCreateDebugObject, (PVOID*)&OriginalNtCreateDebugObject);
	hook_function(g_SymbolsData.NtDebugActiveProcess, NtDebugActiveProcess, (PVOID*)&OriginalNtDebugActiveProcess);
	hook_function(g_SymbolsData.DbgkForwardException, DbgkForwardException, (PVOID*)&OriginalDbgkForwardException);
	hook_function(g_SymbolsData.DbgkMapViewOfSection, DbgkMapViewOfSection, (PVOID*)&OriginalDbgkMapViewOfSection);
	hook_function(g_SymbolsData.DbgkUnMapViewOfSection, DbgkUnMapViewOfSection, (PVOID*)&OriginalDbgkUnMapViewOfSection);
	hook_function(g_SymbolsData.DbgkpSetProcessDebugObject, DbgkpSetProcessDebugObject, (PVOID*)&OriginalDbgkpSetProcessDebugObject);
#endif

	if (!HookDbgkDebugObjectType())
		return FALSE;

	
	return TRUE;
}


BOOLEAN UnHookFuncs()
{
#ifdef WINVM
	DisableIntelVT();
#else
	unhook_all_functions();
#endif
	
	return TRUE;
}