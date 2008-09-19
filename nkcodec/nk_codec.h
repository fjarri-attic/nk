// --------------------------------------------
// ������� ����������� � ������������� ��������
// --------------------------------------------

#include "defines.h"

// �������� �����, ���������������� �������� ������� � ������� vector_num 
// src - ������ ������� length*k, res - ������ ������� length
VOID GFCreateSlice(OUT UCHAR *res, IN UCHAR *src, IN ULONG length, IN UCHAR k, IN UCHAR vector_num);

// ���������� ������� �������� k*k ��� ������� ������ 
// parts_numbers - ������ ������ ������, ������� ������� ����� �������������
// temp - ������� ������ ��� ������������� ����������
// �� ���������� res � temp ������ ���� �������� ������� ������ �������� k*k
VOID GFCreateAssemblingMatrix(OUT UCHAR *res, OUT UCHAR *temp, IN const UCHAR *parts_numbers, IN UCHAR k);

// ������� ��� �������������
// src - ������ ������� length*n, ��� �� ������� �������� ����� ������ length � �������� 1..n
// vector_nums - ������ ������� k � �������� ������, �� ������� ������������ �������������
// assemble_matrix - ������� �������������, ����������� � ������� GFCreateAssemblingMatrix,
// ������, � �������� ������ �� ���������� �� ������ ���� ������� ��� �� ������ vector_nums
VOID GFAssembleSlices(OUT UCHAR *res, IN UCHAR *src, ULONG part_length, UCHAR k, 
					  IN UCHAR *parts_numbers, IN UCHAR *assemble_matrix);