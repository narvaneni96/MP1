#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <bits/stdc++.h>
#include <algorithm>
#include <vector>

using namespace std;

string::size_type sz;
long opCount = 1;
long filePointer = 0;

//Block parameters
struct block
{
    string blockAdd;
    string hexAdd;
    string tag;
    int index;
    bool valid;
    long serial;
    long dirtyBit;
};


//Initial Parameters
long blockSize;
long l1Size;
long l1Assoc;
long l2Size;
long l2Assoc;
long rPolicy;
long iPolicy;
string traceFile;


// Conversion of hex adddress to tag and index
struct address{
    string blockAdd;
    string tag;
    int index;
};

address addMaker(string hexAdd, int blockSize, int numSets){
    long decAdd = stol(hexAdd, &sz, 16);//decimal address of hex address from address structure
    int blockBits = ceil(log2(blockSize));//calculating block offset bits ccccccccccc
    int setBits = ceil(log2(numSets));//index bits as per definition
    long blockAdd  = decAdd % long(pow(2, blockBits));//calculating block offset address in decimal from hex address above
    blockAdd = decAdd - blockAdd;
    long tagAdd = decAdd / long(pow(2, setBits+blockBits));//calculating tag address in decimal
    int index = (decAdd % long(pow(2, setBits+blockBits))) / long(pow(2, blockBits));//calculating tag address in decimal
    address newAdd;
    std::stringstream sstream;
    sstream << std::hex << tagAdd;
    newAdd.tag = sstream.str();
    std::stringstream sstream2;
    sstream2 << std::hex << blockAdd;
    newAdd.blockAdd =  sstream2.str();
    newAdd.index =  index;
    return(newAdd);
}


// This structure will form base for the input content.
struct fileContentStruct{
    string ops;
    string hexAdd;
};


// THis is vector of trace file lines. Each line is one object.
// global vector
vector<fileContentStruct*> traceFileContent;


//creating new struct object out of each line in trace.
//Memory will be allocated in heap.
//globally accessible.
fileContentStruct* getFileContentStruct(string ops, string addr)
{
    fileContentStruct *temp = new fileContentStruct();
    temp->hexAdd = addr;
    temp->ops = ops;
    return temp;
}


// Read Whole trace file and make vector out of it.
void readTraceFile(string traceFileName)
{ 
    ifstream infile(traceFileName);
    string op;
    string hexAdd;
    while(infile >> op >> hexAdd )
    {
        traceFileContent.push_back(getFileContentStruct(op,hexAdd));
    }
}


//Performance parameters
long l1Reads = 0;
long l1ReadMisses = 0;
long l1Writes = 0;
long l1WriteMisses = 0;
long l1MissRate = 0;
long l1Writebacks = 0;
long l2Reads = 0;
long l2ReadMisses = 0;
long l2Writes = 0;
long l2WriteMisses = 0;
long l2MissRate = 0;
long l2WriteBacks = 0;
long totalMemoryTraffic = 0;
long l1DirectWriteBacks = 0;







//Serial Number counters
long l1Serial = 1;
long l2Serial = 1;

//L1 Cache identifier
long l1Sets;
block** l1;

//L2 Cache identifier
long l2Sets;
block**l2;

void l1Read(string hexAdd);

void l1Write(string hexAdd, int write);

void l2Read(string hexAdd);

void l2Write(string hexAdd, int write);

bool l1Invalidate(string hexAdd);

void getBlocksToDeleteForOptimalPolicy(vector<fileContentStruct*> &traceFileContent, int __startIndex, vector<string> &__set, int blockSize, int numSets);


//L1 read function
void l1Read(string hexAdd){

    //Get the tag and index
    address newAdd = addMaker(hexAdd, blockSize, l1Sets);
    
    //Attempt L1 Read
    //cout<<"L1 read : "<<newAdd.blockAdd<<" "<<"(tag "<<newAdd.tag<<", "<<"index "<<newAdd.index<<")"<<endl;
    l1Reads += 1;
    for (int i=0; i<l1Assoc; i++){
        if (l1[newAdd.index][i].tag == newAdd.tag && l1[newAdd.index][i].valid==true){
            //cout<<"L1 hit"<<endl;
            //LRU
            if (rPolicy==0){
                //cout<<"L1 update LRU"<<endl;
                l1[newAdd.index][i].serial = l1Serial++;
            }
            if (rPolicy==1){
                //cout<<"L1 update FIFO"<<endl;
            }
            if (rPolicy==2){
                l1[newAdd.index][i].serial = l1Serial++;
                //cout<<"L1 update optimal"<<endl;
            }
            return;
        }
    }
    //Not present in L1
    l1ReadMisses += 1;
    //l2Reads += 1;
    //cout<<"L1 miss"<<endl;

    //Fetch to L1 cache
    //l1Writes += 1;
    l1Write(hexAdd, 0);
}

// L1 write
void l1Write(string hexAdd, int write){
    
    //Get the tag and index
    address newAdd = addMaker(hexAdd, blockSize, l1Sets);

    //Print exlicit write status is so
    if (write==1){
        //cout<<"L1 write : "<<newAdd.blockAdd<<" "<<"(tag "<<newAdd.tag<<", "<<"index "<<newAdd.index<<")"<<endl;
    }

    //If explicit write
    if (write==1){
        l1Writes += 1;
        //Check if the address is available, then set the dirty bit to true and return
        for (int i=0; i<l1Assoc; i++){
            if (l1[newAdd.index][i].tag == newAdd.tag && l1[newAdd.index][i].valid==true){
                //cout<<"L1 hit"<<endl;
                //LRU
                if (rPolicy==0){
                    //cout<<"L1 update LRU"<<endl;
                    l1[newAdd.index][i].serial = l1Serial++;
                }
                if (rPolicy==1){
                    //cout<<"L1 update FIFO"<<endl;
                }
                if (rPolicy==2){
                    l1[newAdd.index][i].serial = l1Serial++;
                    //cout<<"L1 update optimal"<<endl;
                }
                //Set dirty bit
                l1[newAdd.index][i].dirtyBit = 1;
                //cout<<"L1 set dirty"<<endl;
                return;
            }
        }
        l1WriteMisses += 1;
        //l2Reads += 1;
        //cout<<"L1 miss"<<endl;
    }
    //Else:
    //Determine the victim (empty slot or the minimum serial)
    long victimSerial = INT_MAX;
    if (rPolicy==0 || rPolicy==1){
        for (int i=0; i<l1Assoc; i++){
            if (l1[newAdd.index][i].valid==false){
                victimSerial = l1[newAdd.index][i].serial;
                break;
            }
            else {
                victimSerial = min(victimSerial, l1[newAdd.index][i].serial);
            }
        }
    }

    if (rPolicy==2){
        //Check if an invalid slot is available
        for (int j=0; j<l1Assoc; j++){
            if (l1[newAdd.index][j].valid==false){
                victimSerial = l1[newAdd.index][j].serial;
                break;
            }
        }
        //Determine optimal replacement in case no invalid slots are avialable
        if (victimSerial==INT_MAX){
            vector<string> set;
            for(int j=0; j<l1Assoc; j++){
                set.push_back(l1[newAdd.index][j].blockAdd);
            }

            getBlocksToDeleteForOptimalPolicy(traceFileContent, filePointer + 1, set, blockSize, l1Sets);
            if (set.size()>1){
                //cout<<"Arbitrary"<<endl;
            }
            string victimBlockAdd = set[0];
            for (int j=0; j<l1Assoc; j++){
                if (l1[newAdd.index][j].blockAdd==victimBlockAdd){
                    victimSerial = l1[newAdd.index][j].serial;
                    break;
                }
            }
        }
    }
    
    //Insert in the slot
    for (int i=0; i<l1Assoc; i++){
        if (l1[newAdd.index][i].serial==victimSerial){
            //If empty slot
            if (l1[newAdd.index][i].valid==false){
                //cout<<"L1 victim: none"<<endl;
            }
            else{
                //if victim dirty bit is set, send write to L2
                if (l1[newAdd.index][i].dirtyBit==1){
                    //cout<<"L1 victim: "<<l1[newAdd.index][i].blockAdd<<" (tag "<<l1[newAdd.index][i].tag<<", index "<<l1[newAdd.index][i].index<<", dirty"<<")"<<endl;
                    l1[newAdd.index][i].valid = false;
                    l1Writebacks += 1;
                    l2Write(l1[newAdd.index][i].hexAdd, 1);   
                }
                else{
                    //cout<<"L1 victim: "<<l1[newAdd.index][i].blockAdd<<" (tag "<<l1[newAdd.index][i].tag<<", index "<<l1[newAdd.index][i].index<<", clean"<<")"<<endl;
                    l1[newAdd.index][i].valid = false;
                }
            }

            //Issue a read request to L2
            l2Read(hexAdd);
            
            l1[newAdd.index][i].valid = true;
            l1[newAdd.index][i].hexAdd = hexAdd;
            l1[newAdd.index][i].blockAdd = newAdd.blockAdd;
            l1[newAdd.index][i].tag = newAdd.tag;
            l1[newAdd.index][i].index = newAdd.index;
            l1[newAdd.index][i].serial = l1Serial++;
            if (rPolicy==0){
                //cout<<"L1 update LRU"<<endl;
            }
            if (rPolicy==1){
                //cout<<"L1 update FIFO"<<endl;
            }
            if (rPolicy==2){
                //cout<<"L1 update optimal"<<endl;
            }
            // If write, then set dirty bit, else just read, so don't set dirty bit
            if (write==1){
                //cout<<"L1 set dirty"<<endl;
                l1[newAdd.index][i].dirtyBit = 1;
            }
            else {
                l1[newAdd.index][i].dirtyBit = 0;
            }
            break;
        }
    }
}

//L2 read function
void l2Read(string hexAdd){
    if (l2Size==0){
        return;
    }
    
    //Get the tag and index
    address newAdd = addMaker(hexAdd, blockSize, l2Sets);
    
    //Attempt L2 Read
    //cout<<"L2 read : "<<newAdd.blockAdd<<" "<<"(tag "<<newAdd.tag<<", "<<"index "<<newAdd.index<<")"<<endl; 
    l2Reads += 1;
    for (int i=0; i<l2Assoc; i++){
        if (l2[newAdd.index][i].tag == newAdd.tag && l2[newAdd.index][i].valid==true){
            //cout<<"L2 hit"<<endl;
            //LRU
            if (rPolicy==0){
                //cout<<"L2 update LRU"<<endl;
                l2[newAdd.index][i].serial = l2Serial++;
            }
            if (rPolicy==1){
                //cout<<"L1 update FIFO"<<endl;
            }
            if (rPolicy==2){
                //cout<<"L2 update optimal"<<endl;
                l2[newAdd.index][i].serial = l2Serial++;
            }
            return;
        }
    }
    //Not present in L2
    l2ReadMisses += 1;
    //cout<<"L2 miss"<<endl;

    //Fetch to L2 caches
    l2Write(hexAdd, 0);
}

//L2 write
void l2Write(string hexAdd, int write){
    if (l2Size==0){
        return;
    }
    
    //Get the tag and index
    address newAdd = addMaker(hexAdd, blockSize, l2Sets);

    if (write==1){
        //cout<<"L2 write : "<<newAdd.blockAdd<<" (tag "<<newAdd.tag<<", index "<<newAdd.index<<")"<<endl;
        l2Writes += 1;
    }

    //If explicit write
    if (write==1){
        //Check if the address is available, then set the dirty bit to true and return
        for (int i=0; i<l2Assoc; i++){
            if (l2[newAdd.index][i].tag == newAdd.tag && l2[newAdd.index][i].valid==true){
                //cout<<"L2 hit"<<endl;
                //LRU
                if (rPolicy==0)
                {
                    //cout<<"L2 update LRU"<<endl;
                    l2[newAdd.index][i].serial = l2Serial++;
                }
                if (rPolicy==1){
                //cout<<"L2 update FIFO"<<endl;
                }
                if (rPolicy==2){
                //cout<<"L2 update optimal"<<endl;
                l2[newAdd.index][i].serial = l2Serial++;
                }
                //cout<<"L2 set dirty"<<endl;
                l2[newAdd.index][i].dirtyBit = 1;
                return;
            }
        }
        l2WriteMisses += 1;
        //cout<<"L2 miss"<<endl;
    }
    //Else:
    //Determine the victim (empty slot or the minimum serial)
    long victimSerial = INT_MAX;
    
    if (rPolicy==0 || rPolicy==1){
        for (int i=0; i<l2Assoc; i++){
            if (l2[newAdd.index][i].valid==false){
                victimSerial = l2[newAdd.index][i].serial;
                break;
            }
            else {
                victimSerial = min(long(victimSerial), l2[newAdd.index][i].serial);
            }
        }
    }

    if (rPolicy==2){
        //Check if an invalid slot is available
        for (int j=0; j<l1Assoc; j++){
            if (l2[newAdd.index][j].valid==false){
                victimSerial = l2[newAdd.index][j].serial;
                break;
            }
        }
        //Determine optimal replacement in case no invalid slots are avialable
        if (victimSerial==INT_MAX){
            vector<string> set;
            for(int j=0; j<l2Assoc; j++){
                set.push_back(l2[newAdd.index][j].blockAdd);
            }
            getBlocksToDeleteForOptimalPolicy(traceFileContent, filePointer + 1, set, blockSize, l2Sets);
            string victimBlockAdd= set[0];
            for (int j=0; j<l1Assoc; j++){
                if (l2[newAdd.index][j].blockAdd==victimBlockAdd){
                    victimSerial = l2[newAdd.index][j].serial;
                    break;
                }
            }
        }
    }



    //Insert in the slot
    for (int i=0; i<l2Assoc; i++){
        if (l2[newAdd.index][i].serial==victimSerial){
            //Check for valid and print status accordingly
            if (l2[newAdd.index][i].valid==true){
                if (l2[newAdd.index][i].dirtyBit==true){
                    //cout<<"L2 victim: "<<l2[newAdd.index][i].blockAdd<<" (tag "<<l2[newAdd.index][i].tag<<", index "<<l2[newAdd.index][i].index<<", dirty"<<")"<<endl;
                    
                    if (iPolicy==1){
                        bool r = l1Invalidate(l2[newAdd.index][i].hexAdd);
                        if (r==false){
                            l2WriteBacks += 1;
                        }
                    }

                    else {
                        l2WriteBacks += 1;
                    }
                }
                else{
                    //cout<<"L2 victim: "<<l2[newAdd.index][i].blockAdd<<" (tag "<<l2[newAdd.index][i].tag<<", index "<<l2[newAdd.index][i].index<<", clean"<<")"<<endl;
                    if (iPolicy==1){
                        l1Invalidate(l2[newAdd.index][i].hexAdd);
                    }
                }
            }
            else{
                //cout<<"L2 victim: none"<<endl;
            }
            l2[newAdd.index][i].blockAdd = newAdd.blockAdd;
            l2[newAdd.index][i].index = newAdd.index;
            l2[newAdd.index][i].hexAdd = hexAdd;
            l2[newAdd.index][i].tag = newAdd.tag;
            l2[newAdd.index][i].serial = l2Serial++;
            if (rPolicy==0){
                //cout<<"L2 update LRU"<<endl;
            }
            if (rPolicy==1){
                //cout<<"L2 update FIFO"<<endl;
            }
            if (rPolicy==2){
                //cout<<"L2 update optimal"<<endl;
            }

            l2[newAdd.index][i].valid = 1;
            // If write, then set dirty bit, else just read, so don't set dirty bit
            if (write==1){
                //cout<<"L2 set dirty"<<endl;
                l2[newAdd.index][i].dirtyBit = 1;
            }
            else {
                l2[newAdd.index][i].dirtyBit = 0;
            }
            break;
        }
    }
}

//l1 Invalidate
bool l1Invalidate(string hexAdd){
    address newAdd = addMaker(hexAdd, blockSize, l1Sets);
    for (int j=0; j<l1Assoc; j++){
        if (l1[newAdd.index][j].blockAdd==newAdd.blockAdd){
            if (l1[newAdd.index][j].valid == true){
                l1[newAdd.index][j].valid = false;
                if (l1[newAdd.index][j].dirtyBit == 1){
                    //cout<<"L1 invalidated: "<<newAdd.blockAdd<<" (tag "<<newAdd.tag<<", index "<<newAdd.index<<", dirty)"<<endl;
                    //cout<<"L1 writeback to main memory directly"<<endl;
                    //l1Writebacks += 1;
                    l1DirectWriteBacks += 1;
                    return(true);
                }
                if (l1[newAdd.index][j].dirtyBit == 0){
                    //cout<<"L1 invalidated: "<<newAdd.blockAdd<<" (tag "<<newAdd.tag<<", index "<<newAdd.index<<", clean)"<<endl;
                    return(false);
                }
            }
        }
    }
    return(false);
}

void getBlocksToDeleteForOptimalPolicy(vector<fileContentStruct*> &traceFileContent, int __startIndex, vector<string> &__set, int blockSize, int numSets)
{   
   for(int i =__startIndex ; i<traceFileContent.size();i++)
   {
       if(__set.size() == 1)
            return;
        for(int j = 0;j<__set.size();j++)
        {
            address newAdd = addMaker(traceFileContent[i]->hexAdd, blockSize, numSets);
            if(__set[j] == newAdd.blockAdd) //
            {
                __set.erase(__set.begin()+j);
                break;
            }
        }
   }
   return;
}

