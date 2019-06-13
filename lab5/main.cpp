//
//  main.cpp
//  doccument
//
//  Created by 李婧祎 on 2019/6/10.
//  Copyright © 2019 李婧祎. All rights reserved.
//

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

//磁盘总大小为512*64
#define B        512                   //定义存储块的长度B字节
#define L        64                    //定义磁盘的存储块总数L，逻辑号0-(L-1)
#define K        3                  //磁盘的前k 个块是保留区

#define OK       1                     //操作成功
#define ERROR    0                    //操作错误

#define File_Block_Length    (B-3)  //定义文件磁盘块号数组长度
#define File_Name_Length     (B-1)  //定义文件名的长度

//8*8共表示64个逻辑块，即磁盘的总块数：L
#define map_row_num     8           //位图的行数
#define map_cow_num     8           //位图的列数

#define maxDirectoryNumber  49      //定义目录中目录项（文件数量）的最大值

#define Buffer_Length 64            //定义读写缓冲区的大小，用于打开文件表项

typedef struct FileDescriptor//文件描述符
{
    int fileLength;                                        //文件长度。单位：字节
    int file_allocation_blocknumber[File_Block_Length];    //文件分配到的磁盘块号数组
    int file_block_length;                                 //文件分配到的磁盘块号数组实际长度
    int beginpos;                                          //文件的起始坐标位置beginpos=map_cow_num*row + cow
    int rwpointer;                                         //读写指针：初始化为0，随着RWBuffer变化
    char RWBuffer[Buffer_Length];                          //读写缓冲区，存文件的内容
}FileDescriptor;

typedef struct Directory//目录项，此处仅代表文件
{
    int  index;                                             //文件描述符序号
    int  count;                                             //用于目录中记录文件的数量
    char fileName[File_Name_Length];                        //文件名
    int  isFileFlag;                                        //判断是文件和是目录，1为文件，0为目录
    int  isOpenFlag;                                        //判断文件是否打开，1为打开，0为关闭
    FileDescriptor fileDescriptor;                          //文件描述符信息，与index 对应
}Directory;


char ldisk[L][B];                                            //构建磁盘模型

char memory_area[L*(B - K)];                                //定义一个足够长的字符数组
char mem_area[L*(B - K)] = { '\0' };
Directory Directorys[maxDirectoryNumber + 1];                //包括目录在内，共有目录项maxDirectoryNumber+1，Directorys[0]保存目录
int bitMap[map_row_num][map_cow_num];                        // 0:磁盘块空闲；1:磁盘块使用

void Init();
void directory();
void show_Menu();
void show_ldisk();
void show_bitMap();
int isExist(int index);
int close(int index);
int open(char *filename);
int getSub(char filename[]);
int create(char *filename);
int destroy(char *filename);
int show_File(char *filename);
int lseek(int index, int position);
int read(int index, char memory_area[], int count);
int write(int index, char memory_area[], int count);
int save(int L_pos, int B_pos, int bufLen, int sub);
int load(int step_L, int bufLen, int pos, char memory_area[]);

/**
 * 函数名：show_Menu
 * 函数功能：提示菜单选项
 */
void show_Menu(){
    printf("==================菜单===================\n");
    printf("**   1.新建文件\n");
    printf("**   2.查看文件列表\n");
    printf("**   3.查看位图信息\n");
    printf("**   4.查看硬盘使用情况\n");
    printf("**   5.删除文件\n");
    printf("**   6.打开文件\n");
    printf("**   7.关闭文件\n");
    printf("**   8.移动文件\n");
    printf("**   9.查看文件内容\n");
    printf("**   10.修改文件内容\n");
    printf("**   11.查看文件详细信息\n");
    printf("**   0.退出\n");
    printf("=========================================\n");
}

/**
 * 函数名：Init
 * 函数功能：初始化文件系统：初始化位图(0表示空闲，1表示被占用)；初始化0号文件描述符；
 */
void Init(){
    int i, j;//两个计数量
    /*初始化磁盘*/
    for (i = 0; i<L; i++){
        for (j = 0; j<B; j++){
            ldisk[i][j] = '\0';//初始化为空
        }
    }
    /*初始化目录项*/
    for (i = 0; i <= maxDirectoryNumber; i++){
        for (j = 0; j<File_Name_Length; j++){
            Directorys[i].fileName[j] = '\0';    //初始化名字为'\0'
        }
        
        if (i == 0){//第一个为目录
            Directorys[i].index = 0;                //目录文件描述符序号为0
            Directorys[i].isFileFlag = 0;
            Directorys[i].count = 0;
        }
        else{//其他均表示文件
            Directorys[i].index = -1;          //初始化文件描述符序号为-1
            Directorys[i].count = -1;           //其他文件项不记录count值
            Directorys[i].isFileFlag = 1;       //初始化其他目录项均为文件
            Directorys[i].isOpenFlag = 0;       //文件均关闭
            /*配置文件描述符的相关项*/
            
            memset(Directorys[i].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
            //初始化磁盘号数组为-1
            Directorys[i].fileDescriptor.file_block_length = 0;    //初始化实际分配的磁盘号数组长度为0
            Directorys[i].fileDescriptor.fileLength = 0;           //初始化文件长度为0
            Directorys[i].fileDescriptor.beginpos = 0;             //初始化文件初始位置为0
            for (j = 0; j<Buffer_Length; j++){
                Directorys[i].fileDescriptor.RWBuffer[j] = '\0';   //初始化读写缓存为'\0'
            }
            Directorys[i].fileDescriptor.rwpointer = 0;            //初始化读写指针
        }
    }//for
    /*初始化位示图，均初始化为空闲*/
    for (i = 0; i<map_row_num; i++){
        for (j = 0; j<map_cow_num; j++){
            if (i*map_cow_num + j<K)
                bitMap[i][j] = 1;      //K个保留区
            else
                bitMap[i][j] = 0;      //其余区域空闲
        }//for
    }//for
}//Init

/**
 * 函数名：read
 * 函数功能：从指定文件顺序读入count 个字节memarea 指定的内存位置。读操作从文件的读写指针指示的位置开始
 * @param index
 * @param mem_area
 * @param count
 * @return
 */
int read(int index, char memory_area[], int count){
    int sub = isExist(index);
    if (sub == ERROR){
        printf("索引信息有误 !\n");
        return ERROR;
    }
    /*找到文件*/
    /*打开文件进行操作*/
    if (!Directorys[sub].isOpenFlag){
        open(Directorys[sub].fileName);
    }
    /*读ldisk内容并复制到memory_area[]中*/
    int step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];
    //L的起始位置
    int step_ = Directorys[sub].fileDescriptor.file_block_length - 1;     //记录要拷贝块的数量
    /*将文件以B为大小划分后拷贝给mem_area[]*/
    int pos = 0;
    for (int i = 0; i<step_; i++){
        load(step_L, B, pos, memory_area);
        pos += B;
        step_L++;
    }
    /*拷贝ldisk[][]最后一块数据*/
    int strLen = Directorys[sub].fileDescriptor.fileLength - (B*step_);
    load(step_L, strLen, pos, memory_area);
    //printf("文件：%c\n", memory_area[1]);
    //注：分配的文件大小不是真正拷贝的文件大小
    return OK;
}

/**
 * 函数名：write
 * 函数功能：把memarea 指定的内存位置开始的count 个字节顺序写入指定文件。写操作从文件的读写指针指示的位置开始
 * @param index
 * @param mem_area
 * @param count
 * @return
 */
int write(int index, char memory_area[], int count){
    /*根据index找到文件*/
    int sub = isExist(index);
    if (sub == ERROR){
        printf("索引信息有误!\n");
        return ERROR;
    }
    /*找到文件*/
    /*打开文件进行操作*/
    if (!Directorys[sub].isOpenFlag){
        open(Directorys[sub].fileName);
    }
    /*读入要写的内容*/
    int i = 0;
    int step_ = 0;//file_allocation_blocknumber[]下标计数器
    int num = 0;//RWBuffer[]读写次数计数器
    int step_L;
    int step_B;
    int COUNT = count;
    while (count){
        Directorys[sub].fileDescriptor.RWBuffer[i] = memory_area[COUNT - count];
        count--;//count计数递减
        i++;//计数+1
        if (i == Buffer_Length){
            int step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[step_];
            //列坐标
            int step_B = Buffer_Length * num;
            save(step_L, step_B, Buffer_Length, sub);//复制完成
            num++;
            if (num == B / Buffer_Length){//写完一行，进入下一行
                num = 0;
                step_++;
            }
            i = 0;
            //
        }
        if (count == 0){
            step_L = Directorys[sub].fileDescriptor.file_allocation_blocknumber[step_];
            step_B = Buffer_Length * num;
            save(step_L, step_B, i, sub);
            break;
        }
    }//while
    memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
    return OK;
}

/**
 * 函数名：load
 * 函数功能：把文件内容恢复到数组
 */
int load(int step_L, int bufLen, int pos, char memory_area[])
{
    for (int i = 0; i<bufLen; i++)
    {
        memory_area[pos + i] = ldisk[step_L][i];
    }
    return OK;
}

/**
 * 函数名：save
 * 函数功能：把数组ldisk 存储到文件
 */
int save(int L_pos, int B_pos, int bufLen, int sub)
{
    for (int i = 0; i<bufLen; i++)
    {//无复制字符串的相关函数，单独复制，便于调用
        ldisk[L_pos][B_pos + i] = Directorys[sub].fileDescriptor.RWBuffer[i];
    }
    return OK;
}

/**
 * 函数名：isExist
 * 函数功能：判断某个index值是否存在
 * @param index
 * @return
 */
int isExist(int index)
{
    for (int i = 1; i <= maxDirectoryNumber; i++)
    {
        if (Directorys[i].index == index)
        {
            return i;            //存在
        }
    }
    return ERROR;        //不存在
}

/**
 * 函数名：getSub
 * 函数功能：求目录项下标
 * @param filename
 * @return
 */
int getSub(char filename[])
{
    for (int i = 1; i <= maxDirectoryNumber; i++)
    {
        if (strcmp(Directorys[i].fileName, filename) == 0)
        {
            return i;            //存在
        }
    }
    return ERROR;        //不存在
}

/**
 * 函数名：create
 * 函数功能：根据指定的文件名创建新文件
 * @param filename
 * @return
 */
int create(char *filename){
    /*查询该文件是否存在*/
    for (int i = 1; i <= maxDirectoryNumber; i++){
        if (strcmp(Directorys[i].fileName, filename) == 0){
            printf("该文件已存在!\n");
            return ERROR;            //文件存在
        }
    }
    /*文件不存在，更新Directorys值*/
    int sub,i,j;
    //搜索文件列表，碰到第一个index=-1的值
    for (i = 1; i <= maxDirectoryNumber; i++){
        if (Directorys[i].index == -1){
            //Directorys[i]
            sub = i;    //找到一个可以用的位置
            break;
        }
        else if (i == maxDirectoryNumber){
            printf("空间不足!");
            return ERROR;
        }
    }
    /*为sub目录项赋值*/
    strcpy(Directorys[sub].fileName, filename);
    //更新index
    for (i = 1; i <= maxDirectoryNumber; i++){
        if (!isExist(i)){       //如果不存在编号为i的index值
            Directorys[sub].index = i;   //更新index值
            break;                    //更新结束
        }
    }
    /*输入目录项其他项*/
    printf("文件大小 (< 61*512 Byte):");
    scanf("%d", &Directorys[sub].fileDescriptor.fileLength);
    //计算需要空间块数量，用于判断剩余空间是否满足要求
    int L_Counter;
    if (Directorys[sub].fileDescriptor.fileLength%B){
        L_Counter = Directorys[sub].fileDescriptor.fileLength / B + 1;
    }
    else{
        L_Counter = Directorys[sub].fileDescriptor.fileLength / B;
    }
    //查位示图，找到没使用的位置
    for (i = K; i<map_row_num*map_cow_num - L_Counter; i++){
        int outflag = 0;//标记位，用于跳出外循环
        //查找连续的L_Counter块bitMap空间块
        for (int j = 0; j<L_Counter; j++){
            int maprow = (i + j) / map_cow_num;
            int mapcow = (i + j) % map_cow_num;
            if (bitMap[maprow][mapcow]){//如果位图某块不空闲
                break;//进入外循环继续查找
            }
            else{//位图空闲
                if (j == L_Counter - 1){
                    outflag = 1;//找到可用块，可以跳出查找
                }
            }
        }//for
        if (outflag == 1){
            //为块数组赋值，保存数据
            Directorys[sub].fileDescriptor.file_block_length = L_Counter;
            //    printf("L_Counter=%d\n",L_Counter);
            Directorys[sub].fileDescriptor.beginpos = i;//i为起始位置（起始块号）
            for (j = 0; j<L_Counter; j++){               //数组记录占用的块号
                Directorys[sub].fileDescriptor.file_allocation_blocknumber[j] = i + j;
            }
            //初始化其他项（应该可以省略）
            Directorys[sub].isOpenFlag = 0;
            Directorys[sub].fileDescriptor.rwpointer = 0;
            memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
            break;
        }
        else if (L_Counter + i == map_row_num*map_cow_num - 1 - K){
            printf("空间不足...x\n");
            Directorys[sub].index = -1;
            return ERROR;
        }
    }//for
    int map_ = i;
    printf(" %s 创建成功!\n", filename);
    /*更新位示图*/
    for (int j = 0; j<Directorys[sub].fileDescriptor.file_block_length; j++){//从i开始更新file_block_length的数据
        int maprow = (map_ + j) / map_cow_num;
        int mapcow = (map_ + j) % map_cow_num;
        bitMap[maprow][mapcow] = 1;
    }//for
    
    /*文件建立结束*/
    Directorys[0].count++;          //更新文件数量
    return OK;
}

/**
 * 函数名：destroy
 * 函数功能：根据文件名删除文件
 * @param filename
 * @return
 */
int destroy(char *filename){
    int sub,i;
    for (i = 1; i <= maxDirectoryNumber; i++){//不是顺序的
        if (strcmp(Directorys[i].fileName, filename) == 0){
            sub = i;
            break;                 //找到该文件
        }
        else if (i == maxDirectoryNumber){
            printf("文件不存在！\n");
            return ERROR;           //搜索结束，未找到对应文件名
        }
    }
    //    printf("%d\n",sub);
    //删除操作
    if (Directorys[sub].isOpenFlag){
        printf("文件已打开，不可删除，请关闭后删除！\n");
        return ERROR;
    }
    /*更新位示图信息*/
    //位示图为一个二维数组，记录的是磁盘块的横坐标值，表示该块磁盘是否空闲
    int position = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];   //起始磁盘号
    for (i = 0; i < Directorys[sub].fileDescriptor.file_block_length; i++){
        int d_row = (position + i) / map_row_num;
        int d_cow = (position + i) % map_row_num;
        bitMap[d_row][d_cow] = 0;
    }//赋值空闲
    
    //更新Directorys[sub]信息
    memset(Directorys[sub].fileName, '\0', File_Name_Length);//文件名初始化
    Directorys[sub].index = -1;          //初始化文件描述符序号为-1
    /*配置文件描述符的相关项*/
    memset(Directorys[sub].fileDescriptor.file_allocation_blocknumber, -1, File_Block_Length);
    //初始化磁盘号数组为-1
    Directorys[sub].fileDescriptor.file_block_length = 0;    //初始化实际分配的磁盘号数组长度为0
    Directorys[sub].fileDescriptor.fileLength = 0;           //初始化文件长度为0
    Directorys[sub].fileDescriptor.beginpos = 0;             //初始化文件初始位置为0
    memset(Directorys[sub].fileDescriptor.RWBuffer, '\0', Buffer_Length);
    Directorys[sub].fileDescriptor.rwpointer = 0;            //初始化读写指针
    
    printf(" %s 删除成功！\n", filename);
    /*文件数量处理*/
    Directorys[0].count--;
    return OK;
}

/**
 * 函数名：open
 * 函数功能：根据文件名打开文件。该函数返回的索引号可用于后续的read, write, lseek, 或close 操作
 * @param filename
 * @return
 */
int open(char *filename){
    int sub;
    //查询所有的目录项
    for (int i = 1; i <= maxDirectoryNumber; i++){
        if (strcmp(Directorys[i].fileName, filename) == 0){
            sub = i;
            break;                 //找到该文件
        }
        else if (i == maxDirectoryNumber){
            return ERROR;           //搜索结束，未找到对应文件名
        }
    }
    Directorys[sub].isOpenFlag = 1;      //打开文件标志
    return OK;
}//open

/**
 * 函数名：close
 * 函数功能：根据打开目录索引号关闭指定文件
 * @param index
 * @return
 */
int close(int index){
    int sub;                               //下角标，表示Directorys[sub]
    for (int i = 1; i <= maxDirectoryNumber; i++){
        if (Directorys[i].index == index){
            sub = i;
            if (!Directorys[i].isOpenFlag){
                printf("文件关闭！\n");
            }
            break;                          //找到
        }
        else if (i == maxDirectoryNumber){                     //是否为最后一个查询的数据
            printf("索引信息有误\n");
            return ERROR;                   //index 数据有错误，找不到该索引
        }
    }//for
    //把缓冲区的内容写入磁盘
    int pos = Directorys[sub].fileDescriptor.file_allocation_blocknumber[0];//纵坐标的起始位置
    for (int i = 0; i<Directorys[sub].fileDescriptor.fileLength; i++){
        int L_Pos = i / B;
        int B_Pos = i % B;
        ldisk[pos + L_Pos][B_Pos] = Directorys[sub].fileDescriptor.RWBuffer[i];
    }
    //释放该文件在打开文件表中对应的表目
    Directorys[sub].isOpenFlag = 0;  //释放，标志位置零
    Directorys[sub].fileDescriptor.rwpointer = 0;//读写指针清除
    //返回状态信息
    return OK;
}//close


/**
 * 函数名：lseek
 * 函数功能：把文件的读写指针移动到pos 指定的位置
 * @param index
 * @param pos
 * @return
 */
int lseek(int index, int position){
    //先找到index代表的数据项
    int sub;                               //下角标，表示Directorys[sub]
    for (int i = 1; i <= maxDirectoryNumber; i++){
        if (Directorys[i].index == index){
            sub = i;
            break;                          //找到
        }
        else if (i == maxDirectoryNumber){                  //是否为最后一个查询的数据
            printf("索引信息有误!\n");
            return ERROR;                   //index 数据有错误，找不到该索引
        }
    }//for
    //找到index对应的元素后，把文件的读写指针移动到position 指定的位置
    Directorys[sub].fileDescriptor.rwpointer = position;
    return OK;                              //处理成功
}//lseek


/**
 * 函数名：directory
 * 函数功能：列表显示所有文件及其长度
 */
void directory(){
    if (Directorys[0].count == 0) {
        printf("无文件\n");
    }
    for (int i = 1; i <= Directorys[0].count; i++){
        if (Directorys[i].index != -1){            //index值有效的文件输出
            printf(" %d ：%s\t\t", i, Directorys[i].fileName);
            printf("大小：%d Byte\n", Directorys[i].fileDescriptor.fileLength);
        }//
    }
}//directory

/**
 * 函数名：show_ldisk
 * 函数功能：显示磁盘信息
 */
void show_ldisk(){
    for (int i = 0; i<L; i++){
        printf("%d:", i);
        printf("%s\n", ldisk[i]);
    }
    printf("\n");
}

/**
 * 函数名：show_File
 * 函数功能：显示文件信息
 * @param filename
 * @return
 */
int show_File(char *filename){
    int sub;
    //查询所有的目录项
    for (int i = 1; i <= maxDirectoryNumber; i++){
        if (strcmp(Directorys[i].fileName, filename) == 0){
            sub = i;
            break;                 //找到该文件
        }
        else if (i == maxDirectoryNumber){
            return ERROR;           //搜索结束，未找到对应文件名
        }
    }
    printf("文件名：%s\n", Directorys[sub].fileName);
    printf("是否打开 (Y:0 N:1)：%d\n", Directorys[sub].isOpenFlag);
    printf("索引: %d\n", Directorys[sub].index);
    printf("大小：%d Byte\n", Directorys[sub].fileDescriptor.fileLength);
    return OK;
}

/**
 * 函数名：show_bitMap
 * 函数功能：显示位图信息
 */
void show_bitMap(){
    for (int i = 0; i<map_row_num; i++){
        for (int j = 0; j<map_cow_num; j++){
            printf("%d  ", bitMap[i][j]);
        }
        printf("\n");
    }//for
}//show_bitMap

/**
 * 函数名：main
 * @param argc
 * @param argv
 * @return
 */
int main(){
    int scanner;
    Init();
    printf("\t\t欢迎使用本文件操作系统-16281134\n");
    show_Menu();
    scanf("%d", &scanner);
    while (scanner != 0){
        switch (scanner){
            case 1:{//创建文件
                char newFile[20];
                printf("新建文件名：  ");
                scanf("%s", &newFile);
                create(newFile);
                show_Menu();
                break;
            }
            case 2:{//输出所有的文件信息
                directory();
                show_Menu();
                break;
            }
            case 3:{//输出位示图
                show_bitMap();
                show_Menu();
                break;
            }
            case 4:{//显示磁盘使用情况
                show_ldisk();
                show_Menu();
                break;
            }
            case 5:{//删除文件
                printf("输入文件名删除文件:");
                char destroyfile[20];
                //    memset(destroyfile,'\0',20);
                scanf("%s", &destroyfile);
                destroy(destroyfile);
                show_Menu();
                break;
            }
            case 6:{//打开文件
                printf("输入文件名打开文件:");
                char openfile[20];
                scanf("%s", &openfile);
                if (!open(openfile)){
                    printf("%s 不存在!\n", openfile);
                }
                else{
                    printf("%s 已打开!\n", openfile);
                }
                show_Menu();
                break;
            }
            case 7:{//关闭文件
                printf("输入文件名关闭文件:");
                char closefile[20];
                scanf("%s", &closefile);
                int sub = getSub(closefile);
                if (sub == 0){
                    printf("%s 不存在!\n", closefile);
                }
                else{
                    close(Directorys[sub].index);
                    printf("%s 已关闭!\n", closefile);
                }
                show_Menu();
                break;
            }
            case 8:{//改变指针的位置
                printf("输入文件名移动文件:");
                char movefile[20];
                int move_pos;
                scanf("%s", &movefile);
                printf("移至（路径）：");
                scanf("%d", &move_pos);
                int sub = getSub(movefile);
                if (sub != 0){
                    if (lseek(Directorys[sub].index, move_pos)){
                        printf(" %s 移动成功!", movefile);
                    }
                    else
                        printf(" %s 移动失败...\n", movefile);
                }//if
                show_Menu();
                break;
            }
            case 9:{//文件读
                printf("输入文件名读取文件:");
                char readfile[20];
                scanf("%s", &readfile);
                int sub = getSub(readfile);
                if (sub != 0){//读入整个文件到mem_area[]
                    read(Directorys[sub].index, memory_area, Directorys[sub].fileDescriptor.fileLength);
                    printf("%s\n", memory_area);
                }
                else{
                    printf("文件名错误...\n");
                }
                show_Menu();
                break;
            }
            case 10:{//文件写
                printf("输入文件名修改文件内容:");
                char writefile[20];
                scanf("%s", &writefile);
                int sub = getSub(writefile);
                if (sub != 0){//读入整个文件到mem_area[]
                    char writebuf[L*(B - K)];//定义一个足够大的数组
                    memset(writebuf, '\0', L*(B - K));
                    printf(">>>:");
                    scanf("%s", &writebuf);
                    int len = 0;
                    for (int i = 0; i<Directorys[sub].fileDescriptor.fileLength; i++){
                        if (writebuf[i] == '\0'){
                            len = i;
                            break;
                        }
                    }
                    write(Directorys[sub].index, writebuf, len);
                }
                else{
                    printf("文件不存在...\n");
                }
                show_Menu();
                break;
            }
            case 11:{
                printf("输入文件名查看文件:\n");
                char seefile[20];
                scanf("%s", &seefile);
                if (getSub(seefile)){//文件名存在
                    show_File(seefile);
                }
                else{
                    printf("文件不存在...\n");
                }
            }
                show_Menu();
        }
        printf("\n");
        scanf("%d", &scanner);
    }
    return 1;
}
