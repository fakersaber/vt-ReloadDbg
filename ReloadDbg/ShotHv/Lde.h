#pragma once

/*
	��ȡHOOKĿ����Ҫ���ֽ���
*/
_IRQL_requires_max_(APC_LEVEL)
ULONG
WINAPI
GetWriteCodeLen(
	_In_ PVOID HookLinerAddress,
	_In_ ULONG_PTR ShellCodeLen
);