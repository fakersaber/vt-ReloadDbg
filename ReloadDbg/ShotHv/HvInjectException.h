#pragma once

// @explain: ע���ж�ָ��
	// @parameter: InterruptionType interruption_type	�ж�����
	// @parameter: InterruptionVector vector	�ж�������		 	
	// @parameter: bool deliver_error_code		�Ƿ��д�����
	// @parameter: ULONG32 error_code			�еĻ�����д
	// @return:  void	�������κ�ֵ
void InjectInterruption(
	_In_ InterruptionType interruption_type,
	_In_ InterruptionVector vector,
	_In_ BOOLEAN deliver_error_code,
	_In_ ULONG32 error_code
);

// ���� MTF
void EnableMTF();

// �ر� MTF
void DisableMTF();

// ���� TF �������Թ���
void EnableTF();

// �ر� TF �������Թ���
void DisableTF();
