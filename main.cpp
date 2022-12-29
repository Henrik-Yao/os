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

#define FILEAREASIZE 900	 // 文件区磁盘块数量
#define SWAPAREASIZE 124	 // 交换区磁盘块数量
#define BLOCKSIZE 320		 // 磁盘块大小320bit
#define BLOCKADDRCOUNT 32	 // 一个磁盘块最多可以容纳32个物理块号
#define ADDRSIZE 10			 // 一个物理块号占用10bit
#define MIXFILE 32 * 32 * 40 // 最大文件大小 32*32*40B
#define MAXFILE 32 * 32 * 40 // 最大文件大小 32*32*40B
bool bitMap[30][30];		 // 位示图
int strb[MAXFILE * 8] = {0};

const int num = 64; // 总内存块个数
int blockNum;		// 物理块总数
int pageNum;		// 物理块中的页面数量

struct PCB
{					   // 页表项
	int id;			   // 内存块id
	int fileStartAddr; // 所属文件磁盘起始地址
	int disk_id;	   // 物理块号
};

typedef struct FCB
{
	string username;   // 用户名
	string fileName;   // 文件名
	int fileSize;	   // 文件大小
	bool fileNature;   // 文件属性 0 只读 1 可读可写
	string createTime; // 文件创建时间
	int fileStartAddr; // 文件起始地址
	int fileIndexAddr; // 文件索引地址
	FCB *next;
} FCB, *FCBptr;

typedef struct MCB
{ // 内存控制块
	PCB pcb;
	int pageID;			  // 页面号
	bool data[BLOCKSIZE]; // pagecontent（大小320bit）
	int stayTime;		  // 页面在物理块中的停留时间（与物理块ID对应）
	bool work;			  // 是否空闲 0空 1不空
} MCB;

MCB b[num];

struct SwapReadData
{
	FCB fcb;
	int blockIndex[FILEAREASIZE]; // 物理块号
	int fileData[32][BLOCKSIZE];  // 文件信息 二进制
	string fileContent;			  // 文件内容
};
struct SwapWriteData
{
	FCB fcb;
	bool IsputDown; // 判断文件所需磁盘块是否超出了空闲磁盘块的数量 0 未超出 1 超出
};

struct BLOCK
{
	bool data[BLOCKSIZE]{};
	bool flag = 0;
};
BLOCK file_data[FILEAREASIZE]; // 文件区
struct SWAP
{
	int fileStartAddr = -1; // 所属文件的起始物理块号 同一个文件一样
	bool addr[10]{};		// 对换区与文件区物理块号映射
	bool data[BLOCKSIZE]{}; // 数据
	bool flag = 0;			// 是否空闲 flag = 1 不空闲
	bool IsInFileArea = 0;	// 是否写入文件区 0 代表未写入或者被修改 1 代表在文件区并且数据一致
};
SWAP swap_data[SWAPAREASIZE]; // 对换区
struct FileInformation
{
	int indexBlock;	   // 索引块号
	int fileStartAddr; // 文件起始物理块号
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
string UserName; // 存放当前用户的用户名

// 为索引(磁盘)表项赋值物理块号
int indexBlockAssignment(int index, int n);
// 为磁盘块赋值 每320bit
void dataBlockAssignment(int idx, int n, int SwapAddr);
// 十进制转换为二进制
vector<int> decimalismToBinary(int num_d);
// 删除交换区中的一个文件
void deleteSwapAreaFile();
// 字符串转换为二进制
string strToBinary(string &str);
// 二进制转换为字符串
string binaryToStr(string str_b);
// 输出位示图
void printBitMap();
// 获取空闲磁盘块数量
int freeAreaCount();
// 位示图法获取一个空闲磁盘块的物理块号 不连续分配
int freeAreaIndex();
// 获取连续的空闲交换区磁盘块起始块号和数量
SwapInformation SwapAreaCount(int amount);
// 删除交换区的一个文件占用的全部磁盘块
void deleteSwapAreaFile();
// 交换区写入数据 仅第一次空文件写入数据时调用
SwapWriteData putfileData(FCB fcb, string str);
// 文件区写入数据并建立二级索引
FileInformation secondaryIndex(int fileIndexAddr, int amount, int startAddr);
// 交换区读出数据
SwapReadData getfileData(FCB fcb);
// 删除指定文件数据
void deletefileData(int firstStartAddr, int indexBlock);
// 格式化
void format();
// 修改文件
SwapWriteData amendFile(FCB fcb, string fileContent);
// 写入数据 一个块为单位
void putBlockData(PCB pcb, string BlockContent);
// 创建一个空文件
int createFreeFile();

void InitMCB();																 // 初始化64块内存块
PCB *InitPCB(int filesize);													 // 每读一个文件到内存都要初始化其页表
int exist(int curpage, int fileStartAddr);									 // 看当前文件的当前页面在不在内存中,在的话返回物理块号
PCB *LRUAlloc(FCB f, PCB *pageTable, int disk_id[], int disk_data[64][320]); // 为文件分配内存空间，返回页表
void ShowPage();															 // 内存块号  所属文件外存块号   页面号  多久没被访问
void ShowPageContent();														 // 此函数用于自查内容，内存块号:  所属文件外存块号   页面号 内容
// void ShowStayTime();//内存块号:多久没被访问
int GetLongestStay();									// 获取在物理块中停留时间最长的页面所在物理块号
void Delete(FCB f);										// 关闭文件（仅对读线程）回收内存和该线程的页表
PCB *ReadToMemory(SwapReadData f_Information);			// 读，将文件存入内存，返回该文件页表
SwapWriteData WriteToMandD(SwapReadData f_Information); // 写，对一个文件进行写操作，写回磁盘,返回写回的数据

/*
创建空文件并写入数据
1. 创建空文件 调用 createFreeFile();√
2. 写入数据   调用 putfileData(FCB fcb, string str);
*/

/*
只读文件操作流程：
1. 将文件调入内存 调用 getfileData(FCB fcb);
2. 线程操作结束后 直接释放内存 无需再写入外存
*/

/*
修改(可读可写)文件操作流程
1. 将文件调入内存 调用 getfileData(FCB fcb);
2. 在内存中执行写操作
3. 线程操作结束   调用 amendFile(FCB fcb, string fileContent);
*/

/*
注意：putfileData 和 amendFile 函数返回值中都有一个 swd.IsPutDown参数
	 该参数说明了文件区是否还有足够的空闲块用于写入数据或者写入修改后的数据
	 swd.IsputDown == 1 说明空闲区不够 此时函数直接结束 需要写入的数据或者修改后的数据并不会写入外存
	 swd.IsputDown == 0 说明空闲区足够
*/

// 字符串转换为二进制
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
// 二进制转换为字符串
string binaryToStr(string str_b)
{
	// 每八位转化成十进制，然后将数字结果转化成字符
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
// 输出位示图
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

// 获取空闲磁盘块数量
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

// 位示图法获取一个空闲磁盘块的物理块号 不连续分配
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

// 文件区写入数据并建立二级索引
FileInformation secondaryIndex(int fileIndexAddr, int amount, int startAddr)
{
	FileInformation fi;
	int idx_index = -1,				// 索引块号
		idx_first = -1,				// 一级索引块号
		idx_second = -1;			// 二级索引块号
	int secondIndexBlockCount = -1; // 用于存储物理块号的索引块数目(一级索引块固定一个，所以只统计二级索引块)
	int total = 0;					// 所需磁盘块数量
	int swapAddr = startAddr;		// 交换区地址

	if (amount % BLOCKADDRCOUNT == 0)
		secondIndexBlockCount = amount / BLOCKADDRCOUNT;
	else
		secondIndexBlockCount = amount / BLOCKADDRCOUNT + 1;
	idx_index = fileIndexAddr;
	fi.indexBlock = idx_index; // 索引块号

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
// 十进制转换为二进制
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
// 为索引(磁盘)表项赋值物理块号
int indexBlockAssignment(int index, int n)
{
	// 标明该磁盘块不空闲
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
// 为磁盘块赋值 每320bit
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

// 获取连续的空闲交换区磁盘块起始块号和数量
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
// 删除交换区的一个文件占用的全部磁盘块
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
// 删除交换区指定文件占用的全部磁盘块
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
// 交换区写入数据
SwapWriteData putfileData(FCB fcb, string str)
{
	SwapWriteData swd;
	swd.fcb = fcb;
	int amount; // 文件占用磁盘数目
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
	int freeBlockCount = freeAreaCount(); // 空闲磁盘块数量
	int indexBlockCount = -1;			  // 所需索引块数目

	if (amount % BLOCKADDRCOUNT == 0)
		indexBlockCount = amount / BLOCKADDRCOUNT + 1;
	else
		indexBlockCount = amount / BLOCKADDRCOUNT + 2;
	// 判断所需磁盘数是否超出空闲磁盘数
	if (freeBlockCount < indexBlockCount + amount)
		swd.IsputDown = 1;
	else
		swd.IsputDown = 0;

	if (fileSize > MIXFILE)
		return swd;

	/*
	四种情况:
	1. fi.filesize <= 32*32*40 &  fi.IsputDown = 0 文件大小正常且空闲磁盘块数量够
	2. fi.filesize > 32*32*40 &  fi.IsputDown = 0 文件过大,但空闲磁盘块数量够
	3. fi.filesize <= 32*32*40 &  fi.IsputDown = 1 文件大小正常,但空闲磁盘块数量不够
	4. fi.filesize > 32*32*40 &  fi.IsputDown = 1 文件过大且空闲磁盘块数量不够
	*/
	// 文件起始物理块号为-1代表这个文件已经创建但是还没有内容
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

// 交换区读出数据
SwapReadData getfileData(FCB fcb)
{
	SwapReadData srd;
	srd.fcb = fcb;
	int amount;
	if (fcb.fileSize % (BLOCKSIZE / 8) == 0)
		amount = fcb.fileSize / (BLOCKSIZE / 8);
	else
		amount = fcb.fileSize / (BLOCKSIZE / 8) + 1;
	// 如果交换区存在该文件数据
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
	// 以下为交换区不存在文件数据
	vector<int> idxs;
	// 获取全部二级索引块存放的物理块号
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
	int swapAddr = startAddr; // 交换区物理块号
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

// 删除指定文件数据
void deletefileData(int firstStartAddr, int indexBlock)
{
	// 删除交换区数据
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
	// 删除文件区数据
	for (int idx : idxs)
	{
		file_data[idx].flag = 0;
		for (int i = 0; i < BLOCKSIZE; i++)
		{
			file_data[idx].data[i] = 0;
		}
	}
	// 删除位示图占用情况
	for (int idx : idxs)
	{
		int row = idx / 30;
		int col = idx % 30;
		bitMap[row][col] = 0;
	}
}
// 创建一个空文件
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
	int amountNow = 0;								   // 修改后的文件占用磁盘块数
	int amountAgo = fileBlockCount(fcb.fileIndexAddr); // 获取未修改前的占用磁盘数
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
	// 删除交换区文件
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
	// 格式化位示图
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 30; j++)
		{
			bitMap[i][j] = 0;
		}
	}
	// 格式化文件区
	for (int i = 0; i < FILEAREASIZE; i++)
	{
		for (int j = 0; j < BLOCKSIZE; j++)
		{
			file_data[i].data[j] = 0;
		}
		file_data[i].flag = 0;
	}
	// 格式化交换区
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

// 写入数据 一个块为单位
void putBlockData(PCB pcb, string BlockContent)
{
	string str_b = BlockContent; // 传的就是二进制字符串
	for (int i = 0; i < MAXFILE * 8; i++)
		strb[i] = 0;
	for (int i = 0; i < str_b.size(); i++)
	{
		strb[i] = str_b[i] - '0';
	}
	for (int i = 0; i < SWAPAREASIZE; i++)
	{
		if (swap_data[i].fileStartAddr == pcb.fileStartAddr)
		{
			int p = i;
			while (swap_data[p].fileStartAddr == pcb.fileStartAddr)
			{
				int idx = 0;
				for (int j = 0; j < 10; j++)
				{
					idx += pow(2, j) * swap_data[p].addr[j];
				}
				if (idx == pcb.disk_id)
				{
					for (int j = 0; j < BLOCKSIZE; j++)
					{
						swap_data[p].data[j] = strb[j];
					}
				}
				p++;
			}
		}
	}
	for (int i = 0; i < BLOCKSIZE; i++)
	{
		file_data[pcb.disk_id].data[i] = strb[i];
	}
	return;
}

void InitMCB()
{
	int i;
	blockNum = num;
	pageNum = 0;
	for (int i = 0; i < blockNum; i++)
	{
		b[i].pageID = -1;
		for (int j = 0; j < BLOCKSIZE; j++)
		{
			b[i].data[j] = 0;
		}
		b[i].stayTime = -1;
		b[i].work = 0;
		b[i].pcb.id = -1;
		b[i].pcb.disk_id = -1;
		b[i].pcb.fileStartAddr = -1;
	}
}

PCB *InitPCB(int fileblocknum)
{
	PCB *pageTable = new PCB[fileblocknum]();
	for (int i = 0; i < fileblocknum; i++)
	{
		pageTable[i].disk_id = -1;
		pageTable[i].fileStartAddr = -1;
		pageTable[i].id = -1;
	}
	return pageTable;
}

int exist(int curpage, int fileStartAddr)
{
	for (int i = 0; i < blockNum; i++)
	{
		if (b[i].pageID == curpage && b[i].pcb.fileStartAddr == fileStartAddr)
			return i;
	}
	return -1;
}

int findspace()
{
	for (int i = 0; i < blockNum; i++)
	{
		if (b[i].pageID == -1)
			return i;
	}
	return -1;
}

PCB *LRUAlloc(FCB f, PCB *pageTable, int disk_id[], bool disk_data[32][320])
{
	int fileblocknum = f.fileSize / 40;
	if (f.fileSize % 40 > 0)
		fileblocknum += 1;

	if (fileblocknum > blockNum - pageNum)
	{
		printf("内存块数不够，发生错误");

		return NULL;
	}
	else if (fileblocknum <= 8)
	{
		for (int i = 0; i < fileblocknum; i++)
		{
			int existnum = exist(i, f.fileStartAddr);
			if (existnum != -1)
			{ // 内存中有该页号
				b[existnum].stayTime = 0;
			}
			else
			{							 // 缺页
				int space = findspace(); // 找第一个内存空闲块号
				pageNum++;
				// 填充页表项
				pageTable[i].id = space;
				pageTable[i].fileStartAddr = f.fileStartAddr;
				pageTable[i].disk_id = disk_id[i];
				// 将相关信息填入b[space]
				b[space].pcb = pageTable[i];
				b[space].pageID = i;
				b[space].stayTime = 0;
				b[space].work = 1;
				for (int j = 0; j < 320; j++)
					b[space].data[j] = disk_data[i][j];
			}
			for (int j = 0; j < blockNum; j++)
			{
				if (j == existnum)
					continue;
				if (b[j].pageID != -1)
					b[j].stayTime++; // 每块非空闲内存块时钟数+1
			}
		}
	}
	else if (fileblocknum > 8)
	{
		for (int i = 0; i < 8; i++)
		{
			int existnum = exist(i, f.fileStartAddr);
			if (existnum != -1)
			{ // 内存中有该页号
				b[existnum].stayTime = 0;
			}
			else
			{							 // 缺页
				int space = findspace(); // 找第一个内存空闲块号
				pageNum++;
				// 填充页表项
				pageTable[i].id = space;
				pageTable[i].fileStartAddr = f.fileStartAddr;
				pageTable[i].disk_id = disk_id[i];
				// 将相关信息填入b[space]
				b[space].pcb = pageTable[i];
				b[space].pageID = i;
				b[space].stayTime = 0;
				b[space].work = 1;
				for (int j = 0; j < 320; j++)
					b[space].data[j] = disk_data[i][j];
			}
			for (int j = 0; j < blockNum; j++)
			{
				if (j == existnum)
					continue;
				if (b[j].pageID != -1)
					b[j].stayTime++; // 每块非空闲内存块时钟数+1
			}
		}
		for (int i = 8; i < fileblocknum; i++)
		{
			pageNum++;
			int space = GetLongestStay(); // 返回staytime最大的内存块号
			cout << "内存中第" << space << "块中磁盘块号为" << b[space].pcb.disk_id << "页号为" << b[space].pageID << "的数据块被换出" << endl;
			// 调用Disk中的函数将换出页面信息保存到外存兑换区
			string swapcontent = ""; // 二进制字符串
			for (int m = 0; m < BLOCKSIZE; m++)
				swapcontent += std::to_string(b[i].data[m]);
			PCB swappcb = b[space].pcb;
			// cout << "换出的字符串："<<swapcontent << endl;
			putBlockData(swappcb, swapcontent); // 保存到兑换区

			// 填充页表项
			pageTable[i].id = space;
			pageTable[i].fileStartAddr = f.fileStartAddr;
			pageTable[i].disk_id = disk_id[i];
			// 将相关信息填入b[space]
			b[space].pcb = pageTable[i];
			b[space].pageID = i;
			b[space].stayTime = 0;
			b[space].work = 1;
			for (int j = 0; j < 320; j++)
				b[space].data[j] = disk_data[i][j];
			for (int j = 0; j < blockNum; j++)
			{
				if (j == space)
					continue;
				if (b[j].pageID != -1)
					b[j].stayTime++; // 每块非空闲内存块时钟数+1
			}
		}
	}

	return pageTable;
}

void ShowPage()
{
	printf("                              *******[64块内存块表]*******\n");
	int i;
	// cout << "内存块号    所属文件外存块号   页面号   多久未访问" << endl;
	for (i = 0; i < blockNum; i++)
	{
		cout << "Block[" << i << "]: " << i << "  "
			 << "\tFileStartAddr[" << i << "]: " << b[i].pcb.fileStartAddr << "  "
			 << "\tPageID[" << i << "]: " << b[i].pageID << "  ";
		if (i < 10)
		{
			cout << "\t  Stay[" << i << "]: " << b[i].stayTime << endl;
		}
		else
		{
			cout << "  Stay[" << i << "]: " << b[i].stayTime << endl;
		}
	}
}

void ShowPageContent()
{
	int i;
	for (i = 0; i < blockNum; i++)
	{
		cout << "Block[" << i << "]: " << b[i].pcb.fileStartAddr << "  " << b[i].pageID << " Content: [";
		for (auto j : b[i].data)
			cout << j;
		cout << "]" << endl;
	}
	cout << "--------------" << endl;
}

int GetLongestStay()
{
	int i;
	int max_pos = 0;
	for (i = 0; i < pageNum; i++)
		if (b[max_pos].stayTime < b[i].stayTime)
			max_pos = i;
	return max_pos;
}

void Delete(FCB f)
{
	int fileblocknum = f.fileSize / 40;
	if (f.fileSize % 40 > 0)
		fileblocknum += 1;
	for (int i = 0; i < pageNum; i++)
		for (int j = 0; j < fileblocknum; j++)
		{
			if (b[i].pcb.fileStartAddr == f.fileStartAddr && b[i].pageID == j)
			{
				b[i].pageID = -1;
				for (int j = 0; j < BLOCKSIZE; j++)
				{
					b[i].data[j] = 0;
				}
				b[i].stayTime = -1;
				b[i].work = 0;
				b[i].pcb.fileStartAddr = -1;
				pageNum--;
			}
		}
}

PCB *ReadToMemory(SwapReadData f_Information)
{
	// cout << "文件内容是： " << f_Information.fileContent << endl;
	FCB f1 = f_Information.fcb;
	int fileblocknum1 = f1.fileSize / 40;
	if (f1.fileSize % 40 > 0)
		fileblocknum1 += 1;
	int disk_id[32];
	for (int i = 0; i < 32; i++)
		disk_id[i] = f_Information.blockIndex[i];
	bool disk_data[32][320];
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 320; j++)
			disk_data[i][j] = f_Information.fileData[i][j];
	PCB *pageTable = InitPCB(fileblocknum1);
	// 分配内存并更新MCB和该文件pageTable，（如需要）完成全局LRU置换块
	pageTable = LRUAlloc(f1, pageTable, disk_id, disk_data);
	return pageTable;
}

SwapWriteData WriteToMandD(SwapReadData f_Information)
{

	// 只修改内容和文件大小
	printf("输入你想要写入的内容：\n");
	string content;
	cin >> content;
	cout << "修改后文件内容是：" << content << endl;
	FCB f = f_Information.fcb;
	// int disk_id[32];
	// for (int i = 0; i < 32; i++) disk_id[i] = f_Information.fileContent[i];
	bool disk_data[32][320];
	string content_b = strToBinary(content);
	// for (int i = 0; i < 32; i++)
	//	for (int j = 0; j < 320; j++) {
	//		if (i * 320 + j < content_b.size())
	//			disk_data[i][j] = content_b[i * 320 + j] - '0';
	//		else disk_data[i][j] = 0;
	//	}
	int length = content_b.size();
	int filesize = length / 8;
	if (length % 8 > 0)
		filesize += 1;
	f.fileSize = filesize; // 更新大小
	int fileblocknum1 = f.fileSize / 40;
	if (f.fileSize % 40 > 0)
		fileblocknum1 += 1;
	// PCB* pageTable = InitPCB(fileblocknum1);
	// 分配内存并更新MCB和该文件pageTable，（如需要）完成全局LRU置换块
	// pageTable = LRUAlloc(f, pageTable, disk_id, disk_data);
	// 写了文件要把内容写回磁盘！！！
	return amendFile(f, content);
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
// 获取创建时间
string GetNowTime()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sys.wYear; // 年份
	sys.wMonth;
	sys.wDay;
	sys.wHour;
	sys.wMinute;
	sys.wSecond; // 月份

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
	MFD_Link = (MFD *)new MFD; // 带头结点的单向链表
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
	cout << "初始用户user0创建成功！（默认初始密码为123456）" << endl;
}
bool DefaultFiles = true;
void InitFCB()
{
	if (FCB_Link)
	{
		// cout<<"FCB_Link已被初始化！"<<endl;
		return;
	}
	FCB_Link = (FCB *)new FCB; // 带头结点的单向链表
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
	cout << "请输入要重命名的文件名：";
	cin >> FileName;
	while (temp)
	{
		// 	string username;   // 用户名
		//  string fileName;   // 文件名
		//  int fileSize;      // 文件大小
		//  bool fileNature;   // 文件属性 0 只读 1 可读可写
		//  string createTime; // 文件创建时间
		//  int fileStartAddr; // 文件起始地址
		//  int fileIndexAddr; // 文件索引地址
		//  FCB *next;
		if (temp->fileName == FileName)
		{
			cout << "请输入新的文件名：";
			cin >> temp->fileName;
			cout << "重命名成功！" << endl;
			return;
		}
		temp = temp->next;
	}
	if (!temp)
	{
		cout << "该用户未创建该文件，请先创建文件！" << endl;
		return;
	}
	else
	{
		cout << "重命名成功！" << endl;
	}
}
void help()
{
	cout << "*****************************************************************" << endl;
	cout << "*\t\t命令			  说明			*" << endl;
	cout << "*\t\tadduser			新建用户		*" << endl; // 已实现
	cout << "*\t\trmuser			删除用户		*" << endl; // 已实现
	cout << "*\t\tshowuser		显示用户		*" << endl;		// 已实现
	cout << "*\t\tlogin			登录系统		*" << endl;		// 已实现
	cout << "*\t\tll		    	显示目录		*" << endl; // 已实现
	cout << "*\t\tcreate			创建文件		*" << endl; // 创建空文件写好了
	cout << "*\t\trename			重命文件		*" << endl; // 已实现
	cout << "*\t\topen			打开文件		*" << endl;		// 已实现
	cout << "*\t\tread			读取文件		*" << endl;		// 已实现
	cout << "*\t\twrite			写入文件		*" << endl;		// 已实现
	cout << "*\t\tclose			关闭文件		*" << endl;		// 已实现
	cout << "*\t\tdelete			删除文件		*" << endl; // 已实现
	cout << "*\t\tlogout 			注销用户		*" << endl; // 已实现
	cout << "*\t\thelp			帮助菜单		*" << endl;		// 已实现
	cout << "*\t\tcls 			清除屏幕		*" << endl;		// 已实现
	cout << "*\t\tquit			退出系统		*" << endl;		// 已实现
	cout << "*****************************************************************" << endl;
}
void System_Init()
{
	help();
	// 初始化MFD
	InitMFD();
	InitFCB();
	cout << "创建完毕" << endl;
}
void ShowUsers()
{
	MFD *temp = MFD_Link->next;
	cout << "当前用户列表：" << endl;
	cout << "\tID"
		 << "\t用户名" << endl;
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
	cout << "请输入用户名：";
	cin >> UserName;
	cout << "请输入用户" << UserName << "的密码：";
	char ch;
	int p = 0;
	while ((ch = _getch()) != '\r')
	{
		if (ch == 8)
		{
			if (p > 0)
			{
				putchar('\b'); // 替换*字符
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
			putchar('*'); // 在屏幕上打印星号
		}
		UserPassword[p++] = ch;
	}
	UserPassword[p] = '\0';
	PassWord = UserPassword;
	while (temp)
	{
		if (temp->UserName == UserName)
		{
			cout << "用户已存在！" << endl;
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
		 << "用户" << UserName << "创建成功！" << endl;
	// ShowUsers();
}
void rmuser()
{
	string UserName;
	cout << "请输入要删除的用户名：";
	cin >> UserName;
	MFD *temp = MFD_Link;
	MFD *temp1 = MFD_Link->next;
	while (temp1)
	{
		if (temp1->UserName == UserName)
		{
			char password[20];
			string Password;
			cout << "请输入用户" << UserName << "的密码：";
			for (int j = 3; j > 0; j++)
			{
				char ch;
				int p = 0;
				if (j == 1)
				{
					cout << "密码错误，用户删除失败！" << endl;
					return;
				}
				while ((ch = _getch()) != '\r')
				{
					if (ch == 8)
					{
						if (p > 0)
						{
							putchar('\b'); // 替换*字符
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
						putchar('*'); // 在屏幕上打印星号
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
						 << "密码错误！还有" << j - 1 << "次机会！" << endl;
				}
			}
			temp->next = temp1->next;
			UserId[temp1->id] = 0;
			delete temp1;
			cout << endl
				 << "用户" << UserName << "删除成功！" << endl;
			return;
		}
		temp = temp1;
		temp1 = temp1->next;
	}
	cout << "用户" << UserName << "不存在！" << endl;
};
void login()
{
	if (UserName != "")
	{
		cout << "用户" << UserName << "已登录！请注销用户后登录" << endl;
		return;
	}
	string UserName0;
	cout << "请输入用户名：";
	cin >> UserName0;
	MFD *temp = MFD_Link->next;
	while (temp)
	{
		if (temp->UserName == UserName0)
		{
			char password[20];
			string Password;
			cout << "请输入用户" << UserName0 << "的密码：";
			for (int j = 3; j > 0; j++)
			{
				char ch;
				int p = 0;
				if (j == 1)
				{
					cout << "密码错误，用户登录失败！" << endl;
					return;
				}
				while ((ch = _getch()) != '\r')
				{
					if (ch == 8)
					{
						if (p > 0)
						{
							putchar('\b'); // 替换*字符
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
						putchar('*'); // 在屏幕上打印星号
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
						 << "密码错误！还有" << j - 1 << "次机会！" << endl;
				}
			}
			cout << endl
				 << "用户" << UserName << "登录成功！" << endl;
			UserName = UserName0;
			return;
		}
		temp = temp->next;
	}
	cout << "用户" << UserName0 << "不存在！" << endl;
}
void create()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "请输入文件名：";
		cin >> FileName;
		cout << "请输入权限（0表示只读，1表示可读可写）：";
		int mode;
		cin >> mode;
		while (p->next)
		{

			if (p->next->fileName == FileName && p->next->username == UserName)
			{
				cout << "文件" << FileName << "已存在！" << endl;
				return;
			}
			p = p->next;
		}
		// int fileSize;      // 文件大小
		// int fileStartAddr; // 文件起始地址
		// int fileIndexAddr; // 文件索引地址
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
		cout << "文件" << FileName << "创建成功！" << endl;
	}
}
void PrintFCB()
{ // ls
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
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
		cout << "该用户未创建任何文件，请先创建文件！" << endl;
		return;
	}
	cout << "文件名\t\t文件权限\t文件大小\t起始地址\t索引地址\t创建时间" << endl;
	while (p)
	{
		if (p->username == UserName)
		{
			cout << p->fileName;
			if (!p->fileNature)
			{
				cout << "\t\t只读";
			}
			else
			{
				cout << "\t\t可读写";
			}
			cout << "\t\t" << p->fileSize;
			if (p->fileStartAddr == 0)
			{
				cout << "\t\t————";
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
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "请输入文件名：";
		cin >> FileName;
		while (p->next)
		{
			if (p->next->fileName == FileName && p->next->username == UserName)
			{
				cout << "请输入新文件名：";
				cin >> FileName;
				FCB *q = FCB_Link;
				while (q->next)
				{
					if (q->next->fileName == FileName && q->next->username == UserName)
					{
						cout << "文件" << FileName << "已存在！" << endl;
						return;
					}
					q = q->next;
				}
				p->next->fileName = FileName;
				cout << "文件重命名成功！" << endl;
				return;
			}
			p = p->next;
		}
		cout << "文件" << FileName << "不存在！" << endl;
	}
}
void read()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		AFD *p = AFD_Link;
		FCB *q = FCB_Link;
		cout << "请输入要读取的文件名：";
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
				cout << "文件" << FileName << "的内容为";
				if (temp.fileContent == "")
				{
					cout << "空！" << endl;
					return;
				}
				else
				{
					cout << "：" << temp.fileContent << endl;
					return;
				}
			}
			p = p->next;
		}
		cout << "文件" << FileName << "未打开！" << endl;
		return;
	}
}
bool read(string FileName)
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
			cout << "文件" << FileName << "的内容为";
			if (temp.fileContent == "")
			{
				cout << "空！" << endl;
				return true;
			}
			else
			{
				cout << "：" << temp.fileContent << endl;
				return false;
			}
		}
		p = p->next;
	}
	return true;
}
void open()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link->next;
		AFD *q = AFD_Link;
		cout << "请输入要打开的文件名：";
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
						cout << "文件" << FileName << "已经打开！" << endl;
						read(FileName);
						return;
					}
					if (!q->next)
					{
						break;
					}
					q = q->next;
				}
				cout << "请选择以何种方式打开文件（0表示只读，1表示可读可写）：";
				cin >> Mode;
				if (Mode == true && p->fileNature == false)
				{
					cout << "文件" << FileName << "为只读文件，无法以可读可写方式打开！" << endl;
					return;
				}
				SwapReadData srd = getfileData(*p);
				*p = LinkSwapRFCB(srd, *p);
				q->next = (AFD *)new AFD;
				q = q->next;
				q->FileName = p->fileName;
				q->mode = Mode;
				q->Start = p->fileStartAddr;
				q->FileLength = p->fileSize;
				q->next = NULL;
				if (read(FileName))
				{
					return;
				};
				PCB *pageTable0 = ReadToMemory(srd); // 读到内存里
				ShowPage();							 // 展示内存块表
				free(pageTable0);					 // 释放页表
				return;
			}
			// else if (p->fileName == FileName && p->username != UserName)
			// {
			// 	cout << "文件" << FileName << "不是该用户创建，无法打开！" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "文件" << FileName << "不存在！" << endl;
		return;
	}
}
void write()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link->next;
		AFD *q = AFD_Link;
		cout << "请输入要写入的文件名：";
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
							cout << "文件" << FileName << "以只读形式打开，无法写入！" << endl;
							return;
						}
						else
						{
							cout << "请输入要写入的内容：";
							string content;
							cin >> content;
							SwapReadData srd = getfileData(*p);
							*p = LinkSwapRFCB(srd, *p);
							SwapWriteData swd = putfileData(*p, content);
							*p = LinkSwapWFCB(swd, *p);
							SwapWriteData swd2 = amendFile(*p, content);
							*p = LinkSwapWFCB(swd, *p);
							cout << "文件" << FileName << "写入成功！" << endl;
							return;
						}
					}
					if (!q->next)
					{
						break;
					}
					q = q->next;
				}
				cout << "文件" << FileName << "未打开，无法写入！" << endl;
				return;
			}
			// else if (p->fileName == FileName && p->username != UserName)
			// {
			// 	cout << "文件" << FileName << "不是该用户创建，无法写入！" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "文件" << FileName << "不存在！" << endl;
		return;
	}
}
void close()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		AFD *p = AFD_Link;
		FCB *r = FCB_Link->next;
		cout << "请输入要关闭的文件名：";
		cin >> FileName;
		while (p->next)
		{
			if (p->next->FileName == FileName)
			{
				while (r)
				{
					if (r->fileName == FileName && r->username == UserName)
					{
						break;
					}
					r = r->next; // *r就是要关闭文件的FCB
				}
				AFD *q = p->next;
				p->next = q->next;
				delete q;
				Delete(*r); // 调出内存的不归我管（狗头）
				ShowPage();
				cout << "文件" << FileName << "关闭成功！" << endl;
				return;
			}
			p = p->next;
		}
		cout << "文件" << FileName << "未打开，无法关闭！" << endl;
		return;
	}
}
void rm()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else
	{
		string FileName;
		FCB *p = FCB_Link;
		cout << "请输入要删除的文件名：";
		cin >> FileName;
		AFD *q = AFD_Link;
		while (q->next)
		{
			if (q->next->FileName == FileName)
			{
				cout << "文件" << FileName << "已打开，无法删除！" << endl;
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
				cout << "文件" << FileName << "删除成功！" << endl;
				return;
			}
			// else if (p->next->fileName == FileName && p->next->username != UserName)
			// {
			// 	cout << "文件" << FileName << "不是该用户创建，无法删除！" << endl;
			// 	return;
			// }
			p = p->next;
		}
		cout << "文件" << FileName << "不存在！" << endl;
		return;
	}
}
void logout()
{
	if (UserName == "")
	{
		cout << "请先登录！" << endl;
		return;
	}
	else if (AFD_Link->next != NULL)
	{
		cout << "请先关闭所有文件！" << endl;
		return;
	}
	else
	{
		UserName = "";
		cout << "注销成功！" << endl;
		return;
	}
}
void PrintAFD()
{
	cout << "用户" << UserName << "的文件打开表如下：" << endl;
	AFD *q = AFD_Link;
	if (q->next == NULL)
	{
		cout << "文件打开表为空！" << endl;
		return;
	}
	cout << "文件名\t\t文件长度\t文件起始盘块\t文件打开方式" << endl;
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
			cout << "\t\t————";
		}
		else
		{
			cout << "\t\t" << q->Start;
		}
		if (q->mode == true)
		{
			cout << "\t\t可读可写" << endl;
		}
		else
		{
			cout << "\t\t只读" << endl;
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
			cout << "命令错误！请重新输入" << endl;
		}
	}
}

int main()
{
	format();
	InitMCB(); // 初始化64个内存块

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
	string str = "SHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQWSHDGSJGLSJGLSGSEJLGEKGQJVLQWVNMLWQJFGLJQW";
	SwapWriteData ai = putfileData(fcb, str);
	fcb.fileIndexAddr = ai.fcb.fileIndexAddr;
	fcb.fileStartAddr = ai.fcb.fileStartAddr;
	fcb.fileSize = ai.fcb.fileSize;
	System_Init();
	FileSystem();
	return 0;
}