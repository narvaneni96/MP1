#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <cmath>
#include <bits/stdc++.h>
#include <algorithm>
#include <vector>
#include "sim_cache.h"

using namespace std;


int main(int argc, char* argv[])
{
    
    if (argc<=8)
    {
        cout << "Usage: " << "./sim_cache "; 
        cout << "<BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> "; 
        cout << "<REPLACEMENT_POLICY> <INCLUSION_PROPERTY> <trace_file>"<< endl;
        return(0);
    }
    
    /*string blockSizeStr = argv[1];//BLOCKSIZE
    string l1SizeStr = argv[2];//l1 size
    string l1AssocStr = argv[3];//l1 assoc
    string l2SizeStr = argv[4];//l2 size
    string l2AssocStr = argv[5];//l2 assoc
    string rPolicyStr = argv[6];//replacement policy
    string iPolicyStr = argv[7];//inclusive policy
    traceFile = argv[8];//trace file*/

   /* //convert above string inputs to long int values
    blockSize = stol(blockSizeStr, &sz);
    l1Size = stol(l1SizeStr, &sz);
    l1Assoc = stol(l1AssocStr, &sz);
    l2Size = stol(l2SizeStr, &sz);
    l2Assoc = stol(l2AssocStr, &sz);
    rPolicy = stol(rPolicyStr, &sz);
    iPolicy = stol(iPolicyStr, &sz);*/

    //Read configuration
    blockSize = stol(argv[1], &sz);
    l1Size = stol(argv[2], &sz);
    l1Assoc = stol(argv[3], &sz);
    l2Size = stol(argv[4], &sz);
    l2Assoc = stol(argv[5], &sz);
    rPolicy = stol(argv[6], &sz);
    iPolicy = stol(argv[7], &sz);
    traceFile = argv[8];

    //Print configuration
    cout<<"===== Simulator configuration ====="<<"\n";
    cout<<"BLOCKSIZE:             "<<blockSize<<"\n";
    cout<<"L1_SIZE:               "<<l1Size<<"\n";
    cout<<"L1_ASSOC:              "<<l1Assoc<<"\n";
    cout<<"L2_SIZE:               "<<l2Size<<"\n";
    cout<<"L2_ASSOC:              "<<l2Assoc<<"\n";
    if (rPolicy==0){
        cout<<"REPLACEMENT POLICY:    "<<"LRU"<<"\n";
    }
    else if (rPolicy==1){
        cout<<"REPLACEMENT POLICY:    "<<"FIFO"<<"\n";
    }
    else if (rPolicy==2){
        cout<<"REPLACEMENT POLICY:    "<<"optimal"<<"\n";
    }
    
    if (iPolicy==0){
        cout<<"INCLUSION PROPERTY:    "<<"non-inclusive"<<"\n";
    }
    else if (iPolicy==1){
        cout<<"INCLUSION PROPERTY:    "<<"inclusive"<<"\n";
    }
    
    cout<<"trace_file:            "<<traceFile<<"\n";

    //Allocate L1 Cache memory
    l1Sets = l1Size / (blockSize*l1Assoc);
    l1 = new block*[l1Sets];
    for (int i=0; i<l1Sets; i++){
        l1[i] = new block[l1Assoc];
    }
    //Initialize l1 contents
    for (int i=0; i<l1Sets; i++){
        for (int j=0; j<l1Assoc; j++){
            l1[i][j].blockAdd = "";
            l1[i][j].hexAdd = "";
            l1[i][j].index = i;
            l1[i][j].tag = "";
            l1[i][j].valid = false;
            l1[i][j].serial = 0;
            l1[i][j].dirtyBit = 0;
        }
    }

    //Allocate L2 Cache memory
    if (l2Size>0){
        l2Sets = l2Size / (blockSize*l2Assoc);
        l2 = new block*[l2Sets];
        for (int i=0; i<l2Sets; i++){
            l2[i] = new block[l2Assoc];
        }
        //Set default values
        for (int i=0; i<l2Sets; i++){
            for (int j=0; j<l2Assoc; j++){
                l2[i][j].blockAdd = "";
                l2[i][j].hexAdd = "";
                l2[i][j].index = i;
                l2[i][j].tag = "";
                l2[i][j].valid = false;
                l2[i][j].serial = 0;
                l2[i][j].dirtyBit = 0;
            }
        }
    }

    //Read trace file
    readTraceFile(traceFile);
    for(filePointer = 0; filePointer<traceFileContent.size(); filePointer++){
        string op = traceFileContent[filePointer]->ops;//rwpolicy
        string hexAdd = traceFileContent[filePointer]->hexAdd;
        if (op=="r"){
            l1Read(hexAdd);
        }
        if (op=="w"){
            l1Write(hexAdd, 1); 
        }
    }

    //Print L1 content
    cout<<"===== L1 contents ====="<<endl;
    for (int i=0; i<l1Sets; i++){
        cout<<"Set"<<"\t"<<i<<":"<<"\t";
        for (int j=0; j<l1Assoc; j++){
            cout<<l1[i][j].tag;
            if (l1[i][j].dirtyBit == 1){
                cout<<" "<<"D";
            }
            cout<<"\t";
        }
        cout<<endl;
    }

    //Print L2 content
    if (l2Size>0){
        cout<<"===== L2 contents ====="<<endl;
        for (int i=0; i<l2Sets; i++){
            cout<<"Set"<<"\t"<<i<<":"<<"\t";
            for (int j=0; j<l2Assoc; j++){
                cout<<l2[i][j].tag;
                if (l2[i][j].dirtyBit == 1){
                    cout<<" "<<"D\t";
                }
                else{
                    cout<<"\t";
                }
            }
            cout<<endl;
        }
    }

    //Calculations
    float l1MissRate = float(l1ReadMisses + l1WriteMisses) / float(l1Reads + l1Writes);
    float l2MissRate = 0;
    if (l2Size>0){
        l2MissRate = float(l2ReadMisses)/float(l2Reads);
    }
    
    if (l2Size==0){
        totalMemoryTraffic = l1ReadMisses + l1WriteMisses + l1Writebacks;
    }

    if (l2Size>0 && iPolicy==0){
        totalMemoryTraffic = l2ReadMisses + l2WriteMisses + l2WriteBacks;
    }

    if (l2Size>0 && iPolicy==1){
        totalMemoryTraffic = l2ReadMisses + l2WriteMisses + l2WriteBacks + l1DirectWriteBacks;
    }


    //Print Summary Results
    cout<<"===== Simulation results (raw) ====="<<endl;
    cout<<"a. number of L1 reads:        "<<l1Reads<<endl;
    cout<<"b. number of L1 read misses:  "<<l1ReadMisses<<endl;
    cout<<"c. number of L1 writes:       "<<l1Writes<<endl;
    cout<<"d. number of L1 write misses: "<<l1WriteMisses<<endl;
    cout<<"e. L1 miss rate:              "<<fixed<<setprecision(6)<<l1MissRate<<endl;
    cout<<"f. number of L1 writebacks:   "<<l1Writebacks<<endl;
    cout<<"g. number of L2 reads:        "<<l2Reads<<endl;
    cout<<"h. number of L2 read misses:  "<<l2ReadMisses<<endl;
    cout<<"i. number of L2 writes:       "<<l2Writes<<endl;
    cout<<"j. number of L2 write misses: "<<l2WriteMisses<<endl;
    if (l2Size>0){
        cout<<"k. L2 miss rate:              "<<fixed<<setprecision(6)<<l2MissRate<<endl;
    }
    else{
        cout<<"k. L2 miss rate:              "<<int(l2MissRate)<<endl;
    }
    cout<<"l. number of L2 writebacks:   "<<l2WriteBacks<<endl;
    cout<<"m. total memory traffic:      "<<totalMemoryTraffic<<endl;

    return 0;
}
