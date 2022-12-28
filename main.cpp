#include <iostream>
using namespace std;
#include <math.h>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
#include <string>
#include <valarray>
#include <Windows.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define FILEAREASIZE 900	 // �ļ������̿�����
#define SWAPAREASIZE 124	 // ���������̿�����
#define BLOCKSIZE 320		 // ���̿��С320bit
#define BLOCKADDRCOUNT 32	 // һ�����̿�����������32��������
#define ADDRSIZE 10			 // һ��������ռ��10bit
#define MIXFILE 32 * 32 * 40 // ����ļ���С 32*32*40B
#define MAXFILE 32 * 32 * 40 // ����ļ���С 32*32*40B
bool bitMap[30][30];		 // λʾͼ
int strb[MAXFILE * 8] = {0};

typedef struct FCB
{
	string username;   // �û���
	string fileName;   // �ļ���
	int fileSize;	   // �ļ���С
	bool fileNature;   // �ļ����� 0 ֻ�� 1 �ɶ���д
	string createTime; // �ļ�����ʱ��
	int fileStartAddr; // �ļ���ʼ��ַ
	int fileIndexAddr; // �ļ�������ַ
	FCB *next;
} FCB, *FCBptr;

struct SwapReadData
{
	FCB fcb;
	int blockIndex[FILEAREASIZE]; // ������
	int fileData[32][BLOCKSIZE];  // �ļ���Ϣ ������
	string fileContent;			  // �ļ�����
};
struct SwapWriteData
{
	FCB fcb;
	bool IsputDown; // �ж��ļ�������̿��Ƿ񳬳��˿��д��̿������ 0 δ���� 1 ����
};

struct BLOCK
{
	bool data[BLOCKSIZE]{};
	bool flag = 0;
};
BLOCK file_data[FILEAREASIZE]; // �ļ���
struct SWAP
{
	int fileStartAddr = -1; // �����ļ�����ʼ������ ͬһ���ļ�һ��
	bool addr[10]{};		// �Ի������ļ���������ӳ��
	bool data[BLOCKSIZE]{}; // ����
	bool flag = 0;			// �Ƿ���� flag = 1 ������
	bool IsInFileArea = 0;	// �Ƿ�д���ļ��� 0 ����δд����߱��޸� 1 �������ļ�����������һ��
};
SWAP swap_data[SWAPAREASIZE]; // �Ի���
struct FileInformation
{
	int indexBlock;	   // �������
	int fileStartAddr; // �ļ���ʼ������
};
struct SwapInformation
{
	int startAddr;
	int count;
};

int UserId[20] = {0};

typedef struct MFD
{
	string UserName;
	string Password;
	int id;
	struct MFD *next;
} MFD, *MFDptr;
typedef struct AFD
{
	string FileName;
	int FileLength;
	int Start;
	int mode;
	struct AFD *next;
} AFD, *AFDptr;

MFDptr MFD_Link = NULL;
// UFDptr UFD_Link = NULL;
FCBptr FCB_Link = NULL;
AFDptr AFD_Link = NULL;
string UserName; // ��ŵ�ǰ�û����û���

// Ϊ����(����)���ֵ������
int indexBlockAssignment(int index, int n);
// Ϊ���̿鸳ֵ ÿ320bit
void dataBlockAssignment(int idx, int n, int SwapAddr);
// ʮ����ת��Ϊ������
vector<int> decimalismToBinary(int num_d);
// ɾ���������е�һ���ļ�
void deleteSwapAreaFile();
// �ַ���ת��Ϊ������
string strToBinary(string &str);
// ������ת��Ϊ�ַ���
string binaryToStr(string str_b);
// ���λʾͼ
void printBitMap();
// ��ȡ���д��̿�����
int freeAreaCount();
// λʾͼ����ȡһ�����д��̿�������� ����������
int freeAreaIndex();
// ��ȡ�����Ŀ��н��������̿���ʼ��ź�����
SwapInformation SwapAreaCount(int amount);
// ɾ����������һ���ļ�ռ�õ�ȫ�����̿�
void deleteSwapAreaFile();
// ������д������ ����һ�ο��ļ�д������ʱ����
SwapWriteData putfileData(FCB fcb, string str);
// �ļ���д�����ݲ�������������
FileInformation secondaryIndex(int fileIndexAddr, int amount, int startAddr);
// ��������������
SwapReadData getfileData(FCB fcb);
// ɾ��ָ���ļ�����
void deletefileData(int firstStartAddr, int indexBlock);
// ��ʽ��
void format();
// �޸��ļ�
SwapWriteData amendFile(FCB fcb, string fileContent);
// ����һ�����ļ�
int createFreeFile();

/*
�������ļ���д������
1. �������ļ� ���� createFreeFile();��
2. д������   ���� putfileData(FCB fcb, string str);
*/

/*
ֻ���ļ��������̣�
1. ���ļ������ڴ� ���� getfileData(FCB fcb);
2. �̲߳��������� ֱ���ͷ��ڴ� ������д�����
*/

/*
�޸�(�ɶ���д)�ļ���������
1. ���ļ������ڴ� ���� getfileData(FCB fcb);
2. ���ڴ���ִ��д����
3. �̲߳�������   ���� amendFile(FCB fcb, string fileContent);
*/

/*
ע�⣺putfileData �� amendFile ��������ֵ�ж���һ�� swd.IsPutDown����
	 �ò���˵�����ļ����Ƿ����㹻�Ŀ��п�����д�����ݻ���д���޸ĺ������
	 swd.IsputDown == 1 ˵������������ ��ʱ����ֱ�ӽ��� ��Ҫд������ݻ����޸ĺ�����ݲ�����д�����
	 swd.IsputDown == 0 ˵���������㹻
*/

// �ַ���ת��Ϊ������
string strToBinary(string &str)
{
	string binary_string;
	for (auto i : str)
	{
		bitset<8> bs(i);
		binary_string += bs.to_string();
	}
	return binary_string;
}
// ������ת��Ϊ�ַ���
string binaryToStr(string str_b)
{
	// ÿ��λת����ʮ���ƣ�Ȼ�����ֽ��ת�����ַ�
	string str;
	int sum;
	for (int i = 0; i < str_b.size(); i += 8)
	{
		sum = 0;
		for (int j = 0; j < 8; j++)
			if (str_b[i + j] == '1')
				sum = sum * 2 + 1;
			else
				sum = sum * 2;
		str += char(sum);
	}
	return str;
}
// ���λʾͼ
void printBitMap()
{
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			std::cout << bitMap[i][j] << " ";
		}
		std::cout << endl;
	}
}

// ��ȡ���д��̿�����
int freeAreaCount()
{
	int count = 0;
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			if (bitMap[i][j] == 1)
				continue;
			else
				count++;
		}
	}
	return count;
}

// λʾͼ����ȡһ�����д��̿�������� ����������
int freeAreaIndex()
{
	int idx = -1;
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			idx = (i * 30) + j;
			if (bitMap[i][j] == 0)
				return idx;
		}
	}
	return idx;
}

// �ļ���д�����ݲ�������������
FileInformation secondaryIndex(int fileIndexAddr, int amount, int startAddr)
{
	FileInformation fi;
	int idx_index = -1,				// �������
		idx_first = -1,				// һ���������
		idx_second = -1;			// �����������
	int secondIndexBlockCount = -1; // ���ڴ洢�����ŵ���������Ŀ(һ��������̶�һ��������ֻͳ�ƶ���������)
	int total = 0;					// ������̿�����
	int swapAddr = startAddr;		// ��������ַ

	if (amount % BLOCKADDRCOUNT == 0)
		secondIndexBlockCount = amount / BLOCKADDRCOUNT;
	else
		secondIndexBlockCount = amount / BLOCKADDRCOUNT + 1;
	idx_index = fileIndexAddr;
	fi.indexBlock = idx_index; // �������

	for (int i = 0; i < secondIndexBlockCount; i++)
	{
		idx_first = indexBlockAssignment(idx_index, i);
		if (amount % BLOCKADDRCOUNT != 0 && i + 1 == secondIndexBlockCount)
		{
			for (int j = 0; j < amount % BLOCKADDRCOUNT; j++)
			{
				idx_second = indexBlockAssignment(idx_first, j);
				if (total == 0)
					fi.fileStartAddr = idx_second;
				vector<int> idx_b = decimalismToBinary(idx_second);
				int p = 0;
				for (int item : idx_b)
				{
					swap_data[swapAddr].addr[p++] = item;
				}
				dataBlockAssignment(idx_second, j, swapAddr);
				total++;
				swapAddr++;
			}
		}
		else
		{
			for (int j = 0; j < BLOCKADDRCOUNT; j++)
			{
				idx_second = indexBlockAssignment(idx_first, j);
				if (total == 0)
					fi.fileStartAddr = idx_second;
				vector<int> idx_b = decimalismToBinary(idx_second);
				int p = 0;
				for (int item : idx_b)
				{
					swap_data[swapAddr].addr[p++] = item;
				}
				dataBlockAssignment(idx_second, j, swapAddr);
				total++;
				swapAddr++;
			}
		}
	}
	return fi;
}
// ʮ����ת��Ϊ������
vector<int> decimalismToBinary(int num_d)
{
	int temp;
	vector<int> num_b;
	do
	{
		temp = num_d % 2;
		num_d = num_d / 2;
		num_b.push_back(temp);
	} while (num_d != 0);
	return num_b;
}
// Ϊ����(����)���ֵ������
int indexBlockAssignment(int index, int n)
{
	// �����ô��̿鲻����
	if (file_data[index].flag == 0)
	{
		file_data[index].flag = 1;
		int row = index / 30;
		int col = index % 30;
		bitMap[row][col] = 1;
	}
	int idx;
	idx = freeAreaIndex();
	vector<int> idx_b = decimalismToBinary(idx);
	int i = n * 10;
	for (int item : idx_b)
	{
		file_data[index].data[i++] = item;
	}
	return idx;
}
// Ϊ���̿鸳ֵ ÿ320bit
void dataBlockAssignment(int idx, int n, int SwapAddr)
{

	if (file_data[idx].flag == 0)
	{
		file_data[idx].flag = 1;
		int row = idx / 30;
		int col = idx % 30;
		bitMap[row][col] = 1;
	}
	for (int i = 0; i < BLOCKSIZE; i++)
	{
		file_data[idx].data[i] = strb[BLOCKSIZE * n + i];
		swap_data[SwapAddr].data[i] = strb[BLOCKSIZE * n + i];
	}
}

// ��ȡ�����Ŀ��н��������̿���ʼ��ź�����
SwapInformation SwapAreaCount(int amount)
{
	SwapInformation si;
	int count = 0;
	int startAddr = 0;
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].flag == 1)
		{
			count = 0;
			startAddr = i + 1;
		}
		else
			count++;
		if (count == amount)
			break;
	}
	if (count < amount)
	{
		deleteSwapAreaFile();
		SwapAreaCount(amount);
	}
	si.startAddr = startAddr;
	si.count = count;
	return si;
}
// ɾ����������һ���ļ�ռ�õ�ȫ�����̿�
void deleteSwapAreaFile()
{
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].flag == 1)
		{
			int fileStartAddr = swap_data[i].fileStartAddr;
			int p = i;
			while (swap_data[p].fileStartAddr == fileStartAddr)
			{
				swap_data[p].fileStartAddr = -1;
				swap_data[p].flag = 0;
				swap_data[p].IsInFileArea = 0;
				for (int j = 0; j < ADDRSIZE; j++)
				{
					swap_data[p].addr[j] = 0;
				}
				for (int j = 0; j < BLOCKSIZE; j++)
				{
					swap_data[p].data[j] = 0;
				}
				p++;
			}
			break;
		}
	}
}
// ɾ��������ָ���ļ�ռ�õ�ȫ�����̿�
void deleteAppointSwapAreaFile(int fileStartAddr)
{
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].fileStartAddr == fileStartAddr)
		{
			int p = i;
			while (swap_data[p].fileStartAddr == fileStartAddr)
			{
				swap_data[p].fileStartAddr = -1;
				swap_data[p].flag = 0;
				swap_data[p].IsInFileArea = 0;
				for (int j = 0; j < ADDRSIZE; j++)
				{
					swap_data[p].addr[j] = 0;
				}
				for (int j = 0; j < BLOCKSIZE; j++)
				{
					swap_data[p].data[j] = 0;
				}
				p++;
			}
			return;
		}
	}
}
// ������д������
SwapWriteData putfileData(FCB fcb, string str)
{
	SwapWriteData swd;
	swd.fcb = fcb;
	int amount; // �ļ�ռ�ô�����Ŀ
	string str_b = strToBinary(str);
	for (int i = 0; i < str_b.size(); i++)
	{
		strb[i] = str_b[i] - '0';
	}
	if (str_b.size() % BLOCKSIZE == 0)
		amount = str_b.size() / BLOCKSIZE;
	else
		amount = str_b.size() / BLOCKSIZE + 1;

	int fileSize = str_b.size() / 8;

	swd.fcb.fileSize = fileSize;
	swd.fcb.fileStartAddr = -1;
	swd.fcb.fileIndexAddr = -1;
	int freeBlockCount = freeAreaCount(); // ���д��̿�����
	int indexBlockCount = -1;			  // ������������Ŀ

	if (amount % BLOCKADDRCOUNT == 0)
		indexBlockCount = amount / BLOCKADDRCOUNT + 1;
	else
		indexBlockCount = amount / BLOCKADDRCOUNT + 2;
	// �ж�����������Ƿ񳬳����д�����
	if (freeBlockCount < indexBlockCount + amount)
		swd.IsputDown = 1;
	else
		swd.IsputDown = 0;

	if (fileSize > MIXFILE)
		return swd;

	/*
	�������:
	1. fi.filesize <= 32*32*40 &  fi.IsputDown = 0 �ļ���С�����ҿ��д��̿�������
	2. fi.filesize > 32*32*40 &  fi.IsputDown = 0 �ļ�����,�����д��̿�������
	3. fi.filesize <= 32*32*40 &  fi.IsputDown = 1 �ļ���С����,�����д��̿���������
	4. fi.filesize > 32*32*40 &  fi.IsputDown = 1 �ļ������ҿ��д��̿���������
	*/
	// �ļ���ʼ������Ϊ-1��������ļ��Ѿ��������ǻ�û������
	SwapInformation si = SwapAreaCount(amount);
	int startAddr = si.startAddr;
	FileInformation fi = secondaryIndex(fcb.fileIndexAddr, amount, startAddr);
	swd.fcb.fileStartAddr = fi.fileStartAddr;
	swd.fcb.fileIndexAddr = fi.indexBlock;

	for (int i = startAddr; i < startAddr + amount; i++)
	{
		swap_data[i].IsInFileArea = 1;
		swap_data[i].flag = 1;
		swap_data[i].fileStartAddr = fi.fileStartAddr;
	}
	return swd;
}

// ��������������
SwapReadData getfileData(FCB fcb)
{
	SwapReadData srd;
	srd.fcb = fcb;
	int amount;
	if (fcb.fileSize % (BLOCKSIZE / 8) == 0)
		amount = fcb.fileSize / (BLOCKSIZE / 8);
	else
		amount = fcb.fileSize / (BLOCKSIZE / 8) + 1;
	// ������������ڸ��ļ�����
	int y = 0;
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].fileStartAddr == fcb.fileStartAddr)
		{
			string str = "";
			int idx = 0;
			for (int j = i; j < i + amount; j++)
			{
				idx = 0;
				for (int k = 0; k < ADDRSIZE; k++)
				{
					idx += pow(2, k) * swap_data[j].addr[k];
				}
				srd.blockIndex[y] = idx;
				for (int k = 0; k < BLOCKSIZE; k++)
				{
					srd.fileData[y][k] = swap_data[j].data[k];
					str += std::to_string(swap_data[j].data[k]);
				}
				y++;
			}
			srd.fileContent = binaryToStr(str);
			return srd;
		}
	}
	// ����Ϊ�������������ļ�����
	vector<int> idxs;
	// ��ȡȫ�������������ŵ�������
	for (int i = 0; i < BLOCKADDRCOUNT; i++)
	{
		int idx_first = 0;
		for (int j = i * 10; j < i * 10 + 10; j++)
		{
			idx_first += pow(2, j % 10) * file_data[fcb.fileIndexAddr].data[j];
		}
		if (idx_first == 0)
			break;
		for (int n = 0; n < BLOCKADDRCOUNT; n++)
		{
			int idx_second = 0;
			for (int m = n * 10; m < n * 10 + 10; m++)
			{
				idx_second += pow(2, m % 10) * file_data[idx_first].data[m];
			}
			if (idx_second == 0)
				break;
			idxs.push_back(idx_second);
		}
	}
	string fileData = "";
	SwapInformation si = SwapAreaCount(amount);
	int startAddr = si.startAddr, count = si.count;
	int swapAddr = startAddr; // ������������
	for (int i = startAddr; i < startAddr + count; i++)
	{
		swap_data[i].fileStartAddr = fcb.fileStartAddr;
		swap_data[i].IsInFileArea = 1;
		swap_data[i].flag = 1;
	}
	int x = 0;
	for (int idx : idxs)
	{
		for (int i = 0; i < BLOCKSIZE; i++)
		{
			swap_data[swapAddr].data[i] = file_data[idx].data[i];
		}
		int p = 0;
		vector<int> idx_b = decimalismToBinary(idx);
		for (int item : idx_b)
		{
			swap_data[swapAddr].addr[p++] = item;
		}
		swapAddr++;
		srd.blockIndex[x++] = idx;
	}
	x = 0;
	for (int i = startAddr; i < startAddr + count; i++)
	{
		for (int j = 0; j < BLOCKSIZE; j++)
		{
			srd.fileData[x][j] = swap_data[i].data[j];
			fileData += std::to_string(swap_data[i].data[j]);
		}
		x++;
	}
	srd.fileContent = binaryToStr(fileData);
	return srd;
}

// ɾ��ָ���ļ�����
void deletefileData(int firstStartAddr, int indexBlock)
{
	// ɾ������������
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].fileStartAddr == firstStartAddr)
		{
			swap_data[i].fileStartAddr = -1;
			swap_data[i].flag = 0;
			swap_data[i].IsInFileArea = 0;
			for (int j = 0; j < 10; j++)
			{
				swap_data[i].addr[j] = 0;
			}
			for (int j = 0; j < BLOCKSIZE; j++)
			{
				swap_data[i].data[j] = 0;
			}
		}
	}
	vector<int> idxs;
	idxs.push_back(indexBlock);
	for (int i = 0; i < BLOCKADDRCOUNT; i++)
	{
		int idx = 0;
		for (int j = i * 10; j < i * 10 + 10; j++)
		{
			idx += pow(2, j % 10) * file_data[indexBlock].data[j];
		}
		if (idx == 0)
			break;
		idxs.push_back(idx);
		for (int n = 0; n < BLOCKADDRCOUNT; n++)
		{
			int idx2 = 0;
			for (int m = n * 10; m < n * 10 + 10; m++)
			{
				idx2 += pow(2, m % 10) * file_data[idx].data[m];
			}
			if (idx2 == 0)
				break;
			idxs.push_back(idx2);
		}
	}
	// ɾ���ļ�������
	for (int idx : idxs)
	{
		file_data[idx].flag = 0;
		for (int i = 0; i < BLOCKSIZE; i++)
		{
			file_data[idx].data[i] = 0;
		}
	}
	// ɾ��λʾͼռ�����
	for (int idx : idxs)
	{
		int row = idx / 30;
		int col = idx % 30;
		bitMap[row][col] = 0;
	}
}
// ����һ�����ļ�
int createFreeFile()
{
	int idx_index = freeAreaIndex();
	file_data[idx_index].flag = 1;
	int row = idx_index / 30;
	int col = idx_index % 30;
	bitMap[row][col] = 1;
	return idx_index;
}

int fileBlockCount(int indexBlock)
{
	int count = 0;
	for (int i = 0; i < BLOCKADDRCOUNT; i++)
	{
		int idx = 0;
		for (int j = i * 10; j < i * 10 + 10; j++)
		{
			idx += pow(2, j % 10) * file_data[indexBlock].data[j];
		}
		if (idx == 0)
			break;

		for (int n = 0; n < BLOCKADDRCOUNT; n++)
		{
			int idx2 = 0;
			for (int m = n * 10; m < n * 10 + 10; m++)
			{
				idx2 += pow(2, m % 10) * file_data[idx].data[m];
			}
			if (idx2 == 0)
				break;
			count++;
		}
	}
	return count;
}

SwapWriteData amendFile(FCB fcb, string fileContent)
{
	SwapWriteData swd;
	swd.fcb = fcb;
	string str_b = strToBinary(fileContent);
	for (int i = 0; i < MAXFILE * 8; i++)
		strb[i] = 0;
	for (int i = 0; i < str_b.size(); i++)
	{
		strb[i] = str_b[i] - '0';
	}
	int count = 0;
	int indexBlockCountNow = 0;
	int indexBlockCountAgo = 0;
	int amountNow = 0;								   // �޸ĺ���ļ�ռ�ô��̿���
	int amountAgo = fileBlockCount(fcb.fileIndexAddr); // ��ȡδ�޸�ǰ��ռ�ô�����
	if (str_b.size() % BLOCKSIZE == 0)
		amountNow = str_b.size() / BLOCKSIZE;
	else
		amountNow = str_b.size() / BLOCKSIZE + 1;
	if (amountNow % BLOCKADDRCOUNT == 0)
		indexBlockCountNow = amountNow / BLOCKADDRCOUNT + 1;
	else
		indexBlockCountNow = amountNow / BLOCKADDRCOUNT + 2;
	if (amountAgo % BLOCKADDRCOUNT == 0)
		indexBlockCountAgo = amountAgo / BLOCKADDRCOUNT + 1;
	else
		indexBlockCountAgo = amountAgo / BLOCKADDRCOUNT + 2;

	if (amountNow > amountAgo && (amountNow - amountAgo + indexBlockCountNow - indexBlockCountAgo) > freeAreaCount())
	{
		swd.IsputDown = 1;
		return swd;
	}
	else
	{
		swd.IsputDown = 0;
		swd.fcb.fileSize = str_b.size() / 8;
	}
	// ɾ���������ļ�
	deleteAppointSwapAreaFile(fcb.fileStartAddr);
	SwapInformation si = SwapAreaCount(amountNow);
	int swapAddr = si.startAddr;

	int idxs_first[BLOCKADDRCOUNT];
	int idxs_second[BLOCKADDRCOUNT * BLOCKADDRCOUNT];
	int idxs_first_count = 0, idx_second_count = 0;

	for (int i = 0; i < BLOCKADDRCOUNT; i++)
	{
		int idx = 0;
		for (int j = i * 10; j < i * 10 + 10; j++)
		{
			idx += pow(2, j % 10) * file_data[fcb.fileIndexAddr].data[j];
		}
		if (idx == 0)
			break;
		idxs_first[idxs_first_count++] = idx;
		for (int n = 0; n < BLOCKADDRCOUNT; n++)
		{

			int idx2 = 0;
			for (int m = n * 10; m < n * 10 + 10; m++)
			{
				idx2 += pow(2, m % 10) * file_data[idx].data[m];
			}
			if (idx2 == 0)
				break;
			idxs_second[idx_second_count++] = idx2;
		}
	}
	int p = 0;
	if (amountNow < amountAgo)
	{
		int idxs_first_count_now = 0;
		if (amountNow % BLOCKADDRCOUNT == 0)
			idxs_first_count_now = amountNow % BLOCKADDRCOUNT;
		else
			idxs_first_count_now = amountNow % BLOCKADDRCOUNT + 1;
		if (idxs_first_count_now < idxs_first_count)
		{
			for (int i = idxs_first_count_now * 10; i < idxs_first_count * 10; i++)
			{
				file_data[fcb.fileIndexAddr].data[i] = 0;
			}
		}
		for (int i = 0; i < amountNow; i++)
		{
			int idx = idxs_second[i];
			vector<int> idx_b = decimalismToBinary(idx);
			int q = 0;
			for (int item : idx_b)
			{
				swap_data[swapAddr].addr[q++] = item;
			}
			dataBlockAssignment(idx, p, swapAddr);
			p++;
			swapAddr++;
		}
		for (int i = amountNow; i < amountAgo; i++)
		{
			int idx = idxs_second[i];
			for (int j = 0; j < BLOCKSIZE; j++)
			{
				file_data[idx].data[j] = 0;
			}
			file_data[idx].flag = 0;
		}
	}
	else if (amountNow == amountAgo)
	{
		for (int i = 0; i < amountNow; i++)
		{
			int idx = idxs_second[i];
			vector<int> idx_b = decimalismToBinary(idx);
			int q = 0;
			for (int item : idx_b)
			{
				swap_data[swapAddr].addr[q++] = item;
			}
			dataBlockAssignment(idx, p, swapAddr);
			p++;
			swapAddr++;
		}
	}
	else
	{
		for (int i = 0; i < amountAgo; i++)
		{
			int idx = idxs_second[i];
			vector<int> idx_b = decimalismToBinary(idx);
			int q = 0;
			for (int item : idx_b)
			{

				swap_data[swapAddr].addr[q++] = item;
			}
			dataBlockAssignment(idx, p, swapAddr);
			p++;
			swapAddr++;
		}
		int x = amountAgo % BLOCKADDRCOUNT;
		int y = amountNow - amountAgo;
		if (x != 0)
		{
			int idx_first = idxs_first[idxs_first_count - 1];
			if (y < BLOCKADDRCOUNT - x)
			{
				for (int n = x; n < x + y; n++)
				{
					int idx_second = indexBlockAssignment(idx_first, n);
					vector<int> idx_b = decimalismToBinary(idx_second);
					int q = 0;
					for (int item : idx_b)
					{
						swap_data[swapAddr].addr[q++] = item;
					}

					dataBlockAssignment(idx_second, p, swapAddr);
					p++;
					swapAddr++;
				}
				return swd;
			}
			else
			{
				for (int n = x; n < BLOCKADDRCOUNT; n++)
				{
					int idx_second = indexBlockAssignment(idx_first, n);
					vector<int> idx_b = decimalismToBinary(idx_second);
					int q = 0;
					for (int item : idx_b)
					{
						swap_data[swapAddr].addr[q++] = item;
					}
					dataBlockAssignment(idx_second, p, swapAddr);
					p++;
					swapAddr++;
				}
			}
		}
		int surplusCount = x == 0 ? amountNow - amountAgo : amountNow - amountAgo - (BLOCKADDRCOUNT - x);
		int secondIndexBlockCount = surplusCount % BLOCKADDRCOUNT == 0 ? surplusCount / BLOCKADDRCOUNT : surplusCount / BLOCKADDRCOUNT + 1;

		for (int i = idxs_first_count; i < secondIndexBlockCount + idxs_first_count; i++)
		{
			int idx_first = indexBlockAssignment(fcb.fileIndexAddr, i);
			if (surplusCount % BLOCKADDRCOUNT != 0 && i + 1 == secondIndexBlockCount + idxs_first_count)
			{
				for (int j = 0; j < surplusCount % BLOCKADDRCOUNT; j++)
				{
					int idx_second = indexBlockAssignment(idx_first, j);
					vector<int> idx_b = decimalismToBinary(idx_second);
					int p = 0;
					for (int item : idx_b)
					{
						swap_data[swapAddr].addr[p++] = item;
					}
					dataBlockAssignment(idx_second, j, swapAddr);
					swapAddr++;
				}
			}
			else
			{
				for (int j = 0; j < BLOCKADDRCOUNT; j++)
				{
					int idx_second = indexBlockAssignment(idx_first, j);
					vector<int> idx_b = decimalismToBinary(idx_second);
					int p = 0;
					for (int item : idx_b)
					{
						swap_data[swapAddr].addr[p++] = item;
					}
					dataBlockAssignment(idx_second, j, swapAddr);
					swapAddr++;
				}
			}
		}
	}
	return swd;
}
void format()
{
	// ��ʽ��λʾͼ
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			bitMap[i][j] = 0;
		}
	}
	// ��ʽ���ļ���
	for (int i = 0; i < FILEAREASIZE; i++)
	{
		for (int j = 0; j < BLOCKSIZE; j++)
		{
			file_data[i].data[j] = 0;
		}
		file_data[i].flag = 0;
	}
	// ��ʽ��������
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			swap_data[i].addr[j] = 0;
		}
		for (int j = 0; j < BLOCKSIZE; j++)
		{
			swap_data[i].data[j] = 0;
		}
		swap_data[i].fileStartAddr = -1;
		swap_data[i].flag = 0;
		swap_data[i].IsInFileArea = 0;
	}
}

FCB LinkSwapWFCB(SwapWriteData swd, FCB fcb)
{
	FCB fcb0 = swd.fcb;
	fcb.fileName = fcb0.fileName;
	fcb.fileSize = fcb0.fileSize;
	fcb.fileNature = fcb0.fileNature;
	fcb.fileStartAddr = fcb0.fileStartAddr;
	fcb.fileIndexAddr = fcb0.fileIndexAddr;
	return fcb;
}
FCB LinkSwapRFCB(SwapReadData swd, FCB fcb)
{
	FCB fcb0 = swd.fcb;
	fcb.fileName = fcb0.fileName;
	fcb.fileSize = fcb0.fileSize;
	fcb.fileNature = fcb0.fileNature;
	fcb.fileStartAddr = fcb0.fileStartAddr;
	fcb.fileIndexAddr = fcb0.fileIndexAddr;
	return fcb;
}
// ��ȡ����ʱ��
string GetNowTime()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sys.wYear; // ���
	sys.wMonth;
	sys.wDay;
	sys.wHour;
	sys.wMinute;
	sys.wSecond; // �·�

	string times = "";
	times.append(to_string(sys.wYear)).append("/");
	times.append(to_string(sys.wMonth)).append("/");
	times.append(to_string(sys.wDay)).append(" ");
	times.append(to_string(sys.wHour)).append(":");
	times.append(to_string(sys.wMinute)).append(":");
	times.append(to_string(sys.wSecond)).append(".");
	return times;
}

void InitMFD()
{
	MFD_Link = (MFD *)new MFD; // ��ͷ���ĵ�������
	MFD *p = MFD_Link;
	p->next = (MFD *)new MFD;
	p->next->UserName = "user0";
	p->next->Password = "123456";
	for (int i = 0; i < 20; i++)
	{
		if (UserId[i] == 0)
		{
			UserId[i] = 1;
			p->next->id = i;
			break;
		}
	}
	p = p->next;
	// p->NextUfd = NULL;
	p->next = NULL;
	cout << "��ʼ�û�user0�����ɹ�����Ĭ�ϳ�ʼ����Ϊ123456��" << endl;
}
bool DefaultFiles = true;
void InitFCB()
{
	if (FCB_Link)
	{
		// cout<<"FCB_Link�ѱ���ʼ����"<<endl;
		return;
	}
	FCB_Link = (FCB *)new FCB; // ��ͷ���ĵ�������
	FCB_Link->next = NULL;

}
void InitAFD()
{
	AFD_Link = (AFD *)new AFD;
	AFD_Link->next = NULL;
	return;
}
void RenameFile()
{
	string FileName;
	FCB *temp = FCB_Link;
	cout << "������Ҫ���������ļ�����";
	cin >> FileName;
	while (temp)
	{
		// 	string username;   // �û���
		//  string fileName;   // �ļ���
		//  int fileSize;      // �ļ���С
		//  bool fileNature;   // �ļ����� 0 ֻ�� 1 �ɶ���д
		//  string createTime; // �ļ�����ʱ��
		//  int fileStartAddr; // �ļ���ʼ��ַ
		//  int fileIndexAddr; // �ļ�������ַ
		//  FCB *next;
		if (temp->fileName == FileName)
		{
			cout << "�������µ��ļ�����";
			cin >> temp->fileName;
			cout << "�������ɹ���" << endl;
			return;
		}
		temp = temp->next;
	}
	if (!temp)
	{
		cout << "���û�δ�������ļ������ȴ����ļ���" << endl;
		return;
	}
	else
	{
		cout << "�������ɹ���" << endl;
	}
}
void help()
{
	cout << "*****************************************************************" << endl;
	cout << "*\t\t����			  ˵��			*" << endl;
	cout << "*\t\tadduser			�½��û�		*" << endl; // ��ʵ��
	cout << "*\t\trmuser			ɾ���û�		*" << endl; // ��ʵ��
	cout << "*\t\tshowuser		��ʾ�û�		*" << endl;		// ��ʵ��
	cout << "*\t\tlogin			��¼ϵͳ		*" << endl;		// ��ʵ��
	cout << "*\t\tls		    	��ʾĿ¼		*" << endl; // ��ʵ��
	cout << "*\t\tcreate			�����ļ�		*" << endl; // �������ļ�д����
	cout << "*\t\trename			�����ļ�		*" << endl; // ��ʵ��
	cout << "*\t\topen			���ļ�		*" << endl;		// ��ʵ��
	cout << "*\t\tread			��ȡ�ļ�		*" << endl;		// ��ʵ��
	cout << "*\t\twrite			д���ļ�		*" << endl;		// ��ʵ��
	cout << "*\t\tclose			�ر��ļ�		*" << endl;		// ��ʵ��
	cout << "*\t\tdelete			ɾ���ļ�		*" << endl; // ��ʵ��
	cout << "*\t\tlogout 			ע���û�		*" << endl; // ��ʵ��
	cout << "*\t\thelp			�����˵�		*" << endl;		// ��ʵ��
	cout << "*\t\tcls 			�����Ļ		*" << endl;		// ��ʵ��
	cout << "*\t\tquit			�˳�ϵͳ		*" << endl;		// ��ʵ��
	cout << "*****************************************************************" << endl;
}
void System_Init()
{
	help();
	// ��ʼ��MFD
	InitMFD();
	InitFCB();
	cout << "�������" << endl;
}
void ShowUsers()
{
	MFD *temp = MFD_Link->next;
	cout << "��ǰ�û��б�" << endl;
	cout << "\tID"
		 << "\t�û���" << endl;
	while (temp)
	{
		cout << "\t" << temp->id << "\t" << temp->UserName << endl;
		temp = temp->next;
	}
}
void add_user()
{
	string UserName;
	string PassWord;
	char UserPassword[20];
	MFD *temp = MFD_Link;
	cout << "�������û�����";
	cin >> UserName;
	cout << "�������û�" << UserName << "�����룺";
	char ch;
	int p = 0;
	while ((ch = _getch()) != '\r')
	{
		if (ch == 8)
		{
			if (p > 0)
			{
				putchar('\b'); // �滻*�ַ�
				putchar(' ');
				putchar('\b');
				p--;
			}
			else
			{
				putchar(' ');
				putchar('\b');
			}
			continue;
		}
		else
		{
			putchar('*'); // ����Ļ�ϴ�ӡ�Ǻ�
		}
		UserPassword[p++] = ch;
	}
	UserPassword[p] = '\0';
	PassWord = UserPassword;
	while (temp)
	{
		if (temp->UserName == UserName)
		{
			cout << "�û��Ѵ��ڣ�" << endl;
			return;
		}
		if (!temp->next)
		{
			break;
		}
		temp = temp->next;
	}
	temp->next = (MFD *)new MFD;
	temp = temp->next;
	temp->UserName = UserName;
	temp->Password = PassWord;
	for (int i = 0; i < 20; i++)
	{
		if (UserId[i] == 0)
		{
			UserId[i] = 1;
			temp->id = i;
			break;
		}
	}
	temp->next = NULL;
	// temp->NextUfd = NULL;
	cout << endl
		 << "�û�" << UserName << "�����ɹ���" << endl;
	// ShowUsers();
}
void rmuser()
{
	string UserName;
	cout << "������Ҫɾ�����û�����";
	cin >> UserName;
	MFD *temp = MFD_Link;
	MFD *temp1 = MFD_Link->next;
	while (temp1)
	{
		if (temp1->UserName == UserName)
		{
			char password[20];
			string Password;
			cout << "�������û�" << UserName << "�����룺";
			for (int j = 3; j > 0; j++)
			{
				char ch;
				int p = 0;
				if (j == 1)
				{
					cout << "��������û�ɾ��ʧ�ܣ�" << endl;
					return;
				}
				while ((ch = _getch()) != '\r')
				{
					if (ch == 8)
					{
						if (p > 0)
						{
							putchar('\b'); // �滻*�ַ�
							putchar(' ');
							putchar('\b');
							p--;
						}
						else
						{
							putchar(' ');
							putchar('\b');
						}
						continue;
					}
					else
					{
						putchar('*'); // ����Ļ�ϴ�ӡ�Ǻ�
					}
					password[p++] = ch;
				}
				password[p] = '\0';
				Password = password;
				if (Password == temp1->Password)
				{
					break;
				}
				else
				{
					cout << endl
						 << "������󣡻���" << j - 1 << "�λ��ᣡ" << endl;
				}
			}
			temp->next = temp1->next;
			UserId[temp1->id] = 0;
			delete temp1;
			cout << endl
				 << "�û�" << UserName << "ɾ���ɹ���" << endl;
			return;
		}
		temp = temp1;
		temp1 = temp1->next;
	}
	cout << "�û�" << UserName << "�����ڣ�" << endl;
};
void login()
{
	if (UserName != "")
	{
		cout << "�û�" << UserName << "�ѵ�¼����ע���û����¼" << endl;
		return;
	}
	string UserName0;
	cout << "�������û�����";
	cin >> UserName0;
	MFD *temp = MFD_Link->next;
	while (temp)
	{
		if (temp->UserName == UserName0)
		{
			char password[20];
			string Password;
			cout << "�������û�" << UserName0 << "�����룺";
			for (int j = 3; j > 0; j++)
			{
				char ch;
				int p = 0;
				if (j == 1)
				{
					cout << "��������û���¼ʧ�ܣ�" << endl;
					return;
				}
				while ((ch = _getch()) != '\r')
				{
					if (ch == 8)
					{
						if (p > 0)
						{
							putchar('\b'); // �滻*�ַ�
							putchar(' ');
							putchar('\b');
							p--;
						}
						else
						{
							putchar(' ');
							putchar('\b');
						}
						continue;
					}
					else
					{
						putchar('*'); // ����Ļ�ϴ�ӡ�Ǻ�
					}
					password[p++] = ch;
				}
				password[p] = '\0';
				Password = password;
				if (Password == temp->Password)
				{
					break;
				}
				else
				{
					cout << endl
						 << "������󣡻���" << j - 1 << "�λ��ᣡ" << endl;
				}
			}
			cout << endl
				 << "�û�" << UserName << "��¼�ɹ���" << endl;
			UserName = UserName0;
			return;
		}
		temp = temp->next;
	}
	cout << "�û�" << UserName0 << "�����ڣ�" << endl;
}
void create()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "�������ļ�����";
		cin >> FileName;
		cout << "������Ȩ�ޣ�0��ʾֻ����1��ʾ�ɶ���д����";
		int mode;
		cin >> mode;
		while (p->next)
		{

			if (p->next->fileName == FileName && p->next->username == UserName)
			{
				cout << "�ļ�" << FileName << "�Ѵ��ڣ�" << endl;
				return;
			}
			p = p->next;
		}
		// int fileSize;      // �ļ���С
		// int fileStartAddr; // �ļ���ʼ��ַ
		// int fileIndexAddr; // �ļ�������ַ
		FCB *temp = (FCB *)new FCB;
		temp->fileName = FileName;
		temp->username = UserName;
		string nowtime = GetNowTime();
		temp->createTime = nowtime;
		temp->fileIndexAddr = createFreeFile();
		temp->fileStartAddr = -1;
		temp->fileSize = 0;
		temp->fileNature = (bool)mode;
		temp->next = NULL;
		p->next = temp;
		cout << "�ļ�" << FileName << "�����ɹ���" << endl;
	}
}
void PrintFCB()
{ // ls
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	FCB *p = FCB_Link->next;
	bool is_empty = true;
	while (p)
	{
		if (p->username == UserName)
		{
			is_empty = false;
			break;
		}
		p = p->next;
	}
	if (!p || is_empty)
	{
		cout << "���û�δ�����κ��ļ������ȴ����ļ���" << endl;
		return;
	}
	cout << "�ļ���\t\t�ļ�Ȩ��\t�ļ���С\t��ʼ��ַ\t������ַ\t����ʱ��" << endl;
	while (p)
	{
		if (p->username == UserName)
		{
			cout << p->fileName;
			if (!p->fileNature)
			{
				cout << "\t\tֻ��";
			}
			else
			{
				cout << "\t\t�ɶ�д";
			}
			cout << "\t\t" << p->fileSize;
			if (p->fileStartAddr == 0)
			{
				cout << "\t\t��������";
			}
			else
			{
				cout << "\t\t" << p->fileStartAddr;
			}
			cout << "\t\t" << p->fileIndexAddr << "\t\t" << p->createTime << endl;
		}
		p = p->next;
	}
}
void rename()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "�������ļ�����";
		cin >> FileName;
		while (p->next)
		{
			if (p->next->fileName == FileName && p->next->username == UserName)
			{
				cout << "���������ļ�����";
				cin >> FileName;
				FCB *q = FCB_Link;
				while (q->next)
				{
					if (q->next->fileName == FileName && q->next->username == UserName)
					{
						cout << "�ļ�" << FileName << "�Ѵ��ڣ�" << endl;
						return;
					}
					q = q->next;
				}
				p->next->fileName = FileName;
				cout << "�ļ��������ɹ���" << endl;
				return;
			}
			p = p->next;
		}
		cout << "�ļ�" << FileName << "�����ڣ�" << endl;
	}
}
void read()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		AFD *p = AFD_Link;
		FCB *q = FCB_Link;
		cout << "������Ҫ��ȡ���ļ�����";
		cin >> FileName;
		while (p->next)
		{
			if (p->next->FileName == FileName)
			{
				while (q->next)
				{
					if (q->next->fileName == FileName && q->next->username == UserName)
					{
						break;
					}
					q = q->next;
				}
				SwapReadData temp = getfileData(*(q->next));
				*(q->next) = LinkSwapRFCB(temp, *(q->next));
				cout << "�ļ�" << FileName << "������Ϊ";
				if (temp.fileContent == "")
				{
					cout << "�գ�" << endl;
					return;
				}
				else
				{
					cout << "��" << temp.fileContent << endl;
					return;
				}
			}
			p = p->next;
		}
		cout << "�ļ�" << FileName << "δ�򿪣�" << endl;
		return;
	}
}
void read(string FileName)
{
	AFD *p = AFD_Link;
	FCB *q = FCB_Link;
	while (p->next)
	{
		if (p->next->FileName == FileName)
		{
			while (q->next)
			{
				if (q->next->fileName == FileName && q->next->username == UserName)
				{
					break;
				}
				q = q->next;
			}
			SwapReadData temp = getfileData(*(q->next));
			cout << "�ļ�" << FileName << "������Ϊ";
			if (temp.fileContent == "")
			{
				cout << "�գ�" << endl;
				return;
			}
			else
			{
				cout << "��" << temp.fileContent << endl;
				return;
			}
		}
		p = p->next;
	}
	return;
}
void open()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link->next;
		AFD *q = AFD_Link;
		cout << "������Ҫ�򿪵��ļ�����";
		cin >> FileName;
		bool Mode;
		while (p)
		{
			if (p->fileName == FileName && p->username == UserName)
			{
				while (q->next)
				{
					if (q->next->FileName == FileName)
					{
						cout << "�ļ�" << FileName << "�Ѿ��򿪣�" << endl;
						read(FileName);
						return;
					}
					if (!q->next)
					{
						break;
					}
					q = q->next;
				}
				SwapReadData srd = getfileData(*p);
				*p = LinkSwapRFCB(srd, *p);
				cout << "��ѡ���Ժ��ַ�ʽ���ļ���0��ʾֻ����1��ʾ�ɶ���д����";
				cin >> Mode;
				if (Mode == true && p->fileNature == false)
				{
					cout << "�ļ�" << FileName << "Ϊֻ���ļ����޷��Կɶ���д��ʽ�򿪣�" << endl;
					return;
				}
				q->next = (AFD *)new AFD;
				q = q->next;
				q->FileName = p->fileName;
				q->mode = Mode;
				q->Start = p->fileStartAddr;
				q->FileLength = p->fileSize;
				q->next = NULL;
				read(FileName);
				return;
			}
			// else if (p->fileName == FileName && p->username != UserName)
			// {
			// 	cout << "�ļ�" << FileName << "���Ǹ��û��������޷��򿪣�" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "�ļ�" << FileName << "�����ڣ�" << endl;
		return;
	}
}
void write()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link->next;
		AFD *q = AFD_Link;
		cout << "������Ҫд����ļ�����";
		cin >> FileName;
		while (p)
		{
			if (p->fileName == FileName && p->username == UserName)
			{
				while (q->next)
				{
					if (q->next->FileName == FileName)
					{
						if (q->next->mode == false)
						{
							cout << "�ļ�" << FileName << "��ֻ����ʽ�򿪣��޷�д�룡" << endl;
							return;
						}
						else
						{
							cout << "������Ҫд������ݣ�";
							string content;
							cin >> content;
							SwapReadData srd = getfileData(*p);
							*p = LinkSwapRFCB(srd, *p);
							SwapWriteData swd = putfileData(*p, content);
							*p = LinkSwapWFCB(swd, *p);
							SwapWriteData swd2 = amendFile(*p, content);
							*p = LinkSwapWFCB(swd, *p);
							cout << "�ļ�" << FileName << "д��ɹ���" << endl;
							return;
						}
					}
					if (!q->next)
					{
						break;
					}
					q = q->next;
				}
				cout << "�ļ�" << FileName << "δ�򿪣��޷�д�룡" << endl;
				return;
			}
			// else if (p->fileName == FileName && p->username != UserName)
			// {
			// 	cout << "�ļ�" << FileName << "���Ǹ��û��������޷�д�룡" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "�ļ�" << FileName << "�����ڣ�" << endl;
		return;
	}
}
void close()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		AFD *p = AFD_Link;
		cout << "������Ҫ�رյ��ļ�����";
		cin >> FileName;
		while (p->next)
		{
			if (p->next->FileName == FileName)
			{
				AFD *q = p->next;
				p->next = q->next;
				delete q;
				// �����ڴ�Ĳ����ҹܣ���ͷ��
				cout << "�ļ�" << FileName << "�رճɹ���" << endl;
				return;
			}
			p = p->next;
		}
		cout << "�ļ�" << FileName << "δ�򿪣��޷��رգ�" << endl;
		return;
	}
}
void rm()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "������Ҫɾ�����ļ�����";
		cin >> FileName;
		AFD *q = AFD_Link;
		while (q->next)
		{
			if (q->next->FileName == FileName)
			{
				cout << "�ļ�" << FileName << "�Ѵ򿪣��޷�ɾ����" << endl;
				return;
			}
			q = q->next;
		}
		while (p->next)
		{
			if (p->next->fileName == FileName && p->next->username == UserName)
			{
				FCB *q = p->next;
				p->next = q->next;
				deletefileData(q->fileStartAddr, q->fileIndexAddr);
				delete q;
				cout << "�ļ�" << FileName << "ɾ���ɹ���" << endl;
				return;
			}
			// else if (p->next->fileName == FileName && p->next->username != UserName)
			// {
			// 	cout << "�ļ�" << FileName << "���Ǹ��û��������޷�ɾ����" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "�ļ�" << FileName << "�����ڣ�" << endl;
		return;
	}
}
void logout()
{
	if (UserName == "")
	{
		cout << "���ȵ�¼��" << endl;
		return;
	}
	else if (AFD_Link->next != NULL)
	{
		cout << "���ȹر������ļ���" << endl;
		return;
	}
	else
	{
		UserName = "";
		cout << "ע���ɹ���" << endl;
		return;
	}
}
void PrintAFD()
{
	cout << "�û�" << UserName << "���ļ��򿪱����£�" << endl;
	AFD *q = AFD_Link;
	if (q->next == NULL)
	{
		cout << "�ļ��򿪱�Ϊ�գ�" << endl;
		return;
	}
	cout << "�ļ���\t\t�ļ�����\t�ļ���ʼ�̿�\t�ļ��򿪷�ʽ" << endl;
	// string FileName;
	// int FileLength;
	// int Start;
	// int mode;
	// struct AFD *next;
	while (q->next)
	{
		q = q->next;
		cout << q->FileName << "\t\t" << q->FileLength;
		if (!q->Start)
		{
			cout << "\t\t��������";
		}
		else
		{
			cout << "\t\t" << q->Start;
		}
		if (q->mode == true)
		{
			cout << "\t\t�ɶ���д" << endl;
		}
		else
		{
			cout << "\t\tֻ��" << endl;
		}
	}
}
void FileSystem()
{
	while (1)
	{
		string command;
		cout << UserName << "\\>";
		cin >> command;
		if (command == "adduser")
		{
			add_user();
		}
		else if (command == "rmuser")
		{
			rmuser();
		}
		else if (command == "showuser")
		{
			ShowUsers();
		}
		else if (command == "login")
		{
			login();
			InitAFD();
		}
		else if (command == "logout")
		{
			logout();
		}
		else if (command == "create")
		{
			create();
		}
		else if (command == "ll")
		{
			PrintFCB();
		}
		else if (command == "rename")
		{
			rename();
		}
		else if (command == "open")
		{
			open();
			PrintAFD();
		}
		else if (command == "read")
		{
			read();
		}
		else if (command == "write")
		{
			write();
		}
		else if (command == "close")
		{
			close();
		}
		else if (command == "rm")
		{
			rm();
		}
		else if (command == "cls")
		{
			system("cls");
		}
		else if (command == "help")
		{
			help();
		}
		else if (command == "quit" || command == "exit")
		{
			break;
		}
		else
		{
			cout << "�����������������" << endl;
		}
	}
}

int main()
{
	format();

	FCB fcb;
	InitFCB();
	FCB_Link->next = &fcb;
	fcb.username = "user0";
	fcb.fileName = "a1";
	fcb.fileNature = 1;
	fcb.fileIndexAddr = createFreeFile();
	fcb.fileStartAddr = -1;
	fcb.fileSize = 0;
	fcb.next = NULL;
	fcb.createTime = GetNowTime();
	System_Init();
	FileSystem();
	return 0;
}