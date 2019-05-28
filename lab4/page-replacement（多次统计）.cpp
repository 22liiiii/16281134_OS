//
//  main.cpp
//  page-replacement
//
//  Created by 李婧祎 on 2019/5/16.
//  Copyright © 2019 李婧祎. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/malloc.h>
#include <time.h>
#include "stddef.h"

#define MALLOC_ERROR -1
#define SUCCEED 0

#define BEST_RULE 1
#define RANDOM_RULE 2
#define FIFO_RULE 3
#define LRU_RULE 4
#define CLOCK_RULE 5

#define EMPTY_QUEUE_ERROR NULL

//工作面中包含的页数
#define e 32

//工作面移动率
#define m 20

//剧烈移动在整个移动中的比率
#define t 0.2

//页面总数
int pageNum;

//引用串长度
int rsLen;

//物理内存大小
int frameNum;

int seed = 0;


int* pageArr;//页面算法中的标志位寄存器或者队列
int* rs;//引用串
int* frameArr;//物理内存帧

int bestModeCounter = 0;
int randomModeCounter = 0;
int lruModeCounter = 0;
int clockModeCounter = 0;
int fifoModeCounter = 0;

typedef int RULE;

int init(int pn, int rsl, int fn) {
    pageNum = pn;
    frameNum = fn;
    rsLen = rsl;
    
    rs = (int*)malloc(sizeof(int)*rsLen);
    frameArr = (int*)malloc(sizeof(int)*frameNum);
    
    if (!(rs && frameArr)) { return MALLOC_ERROR; }
    
    int i = 0;
    for (i = 0; i<rsLen; i += 1) { rs[i] = -1; }
    for (i = 0; i<frameNum; i += 1) { frameArr[i] = -1; }
    
    return SUCCEED;
}

int recycle() {
    free(rs);
    free(frameArr);
    
    pageNum = 0;
    rsLen = 0;
    frameNum = 0;
    
    return SUCCEED;
}

void reInit() {
    int i;
    for (i = 0; i<frameNum; i += 1) { frameArr[i] = -1; }
}

void enQueue(int* q, int ele) {
    int i = 0;
    for (i = 0; i<frameNum; i += 1) {
        if (q[i] == -1) { break; }
    }
    q[i] = ele;
}

int deQueue(int* q) {
    int ret = q[0];
    int i;
    for (i = 1; i<frameNum; i += 1) {
        q[i - 1] = q[i];
    }
    q[frameNum - 1] = -1;
    return ret;
}

int produceRs() {
    int p;//p:工作集的起始位置
    int i;
    srand(seed);
    seed += 1;
    p = rand() % pageNum;//产生0到(P-1)之间的随机数
    double r;
    int rp = 0;
    do
    {
        for (i = 0; i<m; i++)
        {
            int x = p + rand() % e;//生成p和p+e间的随机数
            rs[rp] = x%pageNum;
            rp++;
        }
        r = rand() / (RAND_MAX + 1.0);
        if ((t - r)>0.0)
            p = rand() % pageNum;
        else
            p = (p + 1) % pageNum;
    } while (rp < rsLen);
    
    return SUCCEED;
}

int choseToOut(int cur) {
    int i, j;
    int max = -1;
    int maxIndex = -1;
    for (i = 0; i<frameNum; i += 1) {
        for (j = cur + 1; j<rsLen; j += 1) {
            if (rs[j] == frameArr[i]) { break; }
        }
        if (max<j) { max = j; maxIndex = i; }
    }
    return maxIndex;
}


int bestMode()
{
    int i, j, k;
    for (i = 0; i<rsLen; i += 1) {
        //检查有没有在内存里
        int ifInside = 0;
        for (j = 0; j<frameNum; j += 1) {
            if (rs[i] == frameArr[j]) {
                ifInside = 1;
                break;
            }
        }
        
        if (ifInside == 1) {
            continue;
        }
        
        int ifFull = 1;
        for (j = 0; j <frameNum; j += 1) {
            if (frameArr[j] == -1) { frameArr[j] = rs[i]; ifFull = 0; break; }
        }
        
        if (ifFull == 0) { continue; }
        
        
        int chosenFrame = choseToOut(i);
        frameArr[chosenFrame] = rs[i];
        bestModeCounter += 1;
    }
    
    return 0;
}

int randomMode() {
    int i = 0;
    int j = 0;
    srand((int)time(NULL));
    for (i = 0; i<rsLen; i += 1) {
        int cur = rs[i];
        int ifLost = 1;
        for (j = 0; j <frameNum; j += 1) {
            if (cur == frameArr[j]) { ifLost = 0; break; }
        }
        //不缺页
        if (!ifLost) {
            continue;
        }
        else {//缺页
            
            randomModeCounter += 1;
            
            int ifFinish = 0;
            
            for (j = 0; j<frameNum; j += 1) {//物理内存有空位
                if (frameArr[j] == -1) { frameArr[j] = cur; ifFinish = 1; break; }
            }
            
            if (ifFinish) {
                continue;
            }
            
            //物理内存没有空位
            int chosenFrame = rand() % frameNum;//确定要被替换的Frame
            frameArr[chosenFrame] = cur;//替换
        }
    }
    return SUCCEED;
}

int fifoMode() {
    int i = 0;
    int j = 0;
    
    int* q = (int*)malloc(sizeof(int)*frameNum);
    for (i = 0; i<frameNum; i += 1) {
        q[i] = -1;
    }
    
    for (i = 0; i<rsLen; i += 1) {
        int cur = rs[i];
        int ifLost = 1;
        for (j = 0; j <frameNum; j += 1) {
            if (cur == frameArr[j]) { ifLost = 0; break; }
        }
        //不缺页
        if (!ifLost) {
            continue;
        }
        else {//缺页
            
            fifoModeCounter += 1;
            
            int ifFinish = 0;
            
            for (j = 0; j<frameNum; j += 1) {//物理内存有空位
                if (frameArr[j] == -1) { frameArr[j] = cur; ifFinish = 1; break; }
            }
            
            if (ifFinish) {
                enQueue(q, j);
                continue;
            }
            
            //物理内存没有空位
            int chosenFrame = deQueue(q);//确定要被替换的Frame
            frameArr[chosenFrame] = cur;//替换
            enQueue(q, chosenFrame);
        }
    }
    return SUCCEED;
}

int lruMode() {
    int i = 0;
    int j = 0;
    int* counter = (int*)malloc(sizeof(int)*frameNum);
    for (i = 0; i<frameNum; i += 1) {
        counter[i] = 0;
    }
    
    for (i = 0; i<rsLen; i += 1) {
        
        for (j = 0; j<frameNum; j += 1) {
            counter[j] += 1;
        }
        
        int cur = rs[i];
        int ifLost = 1;
        for (j = 0; j <frameNum; j += 1) {
            if (cur == frameArr[j]) { ifLost = 0; break; }
        }
        //不缺页
        if (!ifLost) {
            counter[j] = 0;
            continue;
        }
        else {//缺页
            
            lruModeCounter += 1;
            
            int ifFinish = 0;
            
            for (j = 0; j<frameNum; j += 1) {//物理内存有空位
                if (frameArr[j] == -1) { frameArr[j] = cur; ifFinish = 1; break; }
            }
            
            if (ifFinish) {
                counter[j] = 0;
                continue;
            }
            
            //物理内存没有空位
            int chosenFrame = 0;
            int max = counter[chosenFrame];
            
            //确定要被替换的Frame
            for (j = 1; j<frameNum; j += 1) {
                if (counter[j]>max) { chosenFrame = j; max = counter[j]; }
            }
            frameArr[chosenFrame] = cur;//替换
            counter[chosenFrame] = 0;
        }
    }
    return SUCCEED;
}

int clockMode() {
    int i = 0;
    int j = 0;
    int* flag = (int*)malloc(sizeof(int)*frameNum);
    int flagCur = 0;
    for (i = 0; i<frameNum; i += 1) {
        flag[i] = 0;
    }
    
    for (i = 0; i<rsLen; i += 1) {
        int cur = rs[i];
        int ifInside = 0;
        
        //先判断是不是在里面
        for (j = 0; j<frameNum; j += 1) {
            if (frameArr[j] == cur) { ifInside = 1; break; }
        }
        
        if (ifInside) {
            flag[j] = 1;
            continue;
        }
        
        //如果不在里面
        clockModeCounter += 1;
        while (1) {
            if (flag[flagCur] == 0) { break; }
            else { flag[flagCur] = 0; flagCur += 1; flagCur %= frameNum; }
        }
        frameArr[flagCur] = cur;
    }
    return SUCCEED;
}

void writeToFile() {
    FILE *file;
    //fopen_s(&file, "output.csv", "a");
    file=fopen("output.csv", "a");
    
    fprintf(file, "%f,%f,%f,%f,%f\n", ((float)bestModeCounter / (float)rsLen), ((float)randomModeCounter / (float)rsLen), ((float)fifoModeCounter / (float)rsLen), ((float)lruModeCounter / (float)rsLen), ((float)clockModeCounter / (float)rsLen));
    fclose(file);
}

int test() {
    init(100, 10000, 22);
    produceRs();
    
    reInit();
    randomMode();
    
    reInit();
    lruMode();
    
    reInit();
    fifoMode();
    
    reInit();
    clockMode();
    
    reInit();
    bestMode();
    
    writeToFile();
    recycle();
    
    return 0;
}


void initCounter() {
    bestModeCounter = 0;
    lruModeCounter = 0;
    fifoModeCounter = 0;
    clockModeCounter = 0;
    randomModeCounter = 0;
}

int main() {
    printf("/*****************开始性能评测************************/\n");
    FILE *file;
   // fopen_s(&file, "output.csv", "w");
    file = fopen("output.csv", "w");
    //fclose(file);
    
    //int j = 0;
    for (int i = 0; i<100; i += 1) {
        initCounter();
        test();
    }
    
    printf("/*****************性能评测结束************************/\n");
    //system("pause");
    return 0;
}
