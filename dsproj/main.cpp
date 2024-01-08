//
//  main.cpp
//  dsproj
//
//  Created by 谢文昊 on 2022/12/9.
//

#include <iostream>
#include <fstream>
#include "bvh_parser.h"

// a naive bvh parser

using namespace std;

int counts = 0;

void treeConstructor(ifstream &file,joint* &root){
    string curr;
    file >> curr;
    
    if(curr == "ROOT" || curr == "JOINT"){
        
        root->joint_type = curr;
        file >> root->name;
        
        // load offsets
        do{
            file >> curr;
        }while(curr != "OFFSET");
        file >> root->offset_x >> root->offset_y >> root->offset_z;
        
        // load channels
        do{
            file >> curr;
        }while(curr != "CHANNELS");
        int numberOfChannels;
        file >> numberOfChannels;
        for(int i=0;i<numberOfChannels;i++){
            file >> curr;
            root->channels.push_back(curr);
        }
        
        // set children
        file >> curr;
        while(curr == "JOINT"){
            joint *child_tmp = new joint;
            root->children.push_back(child_tmp);
            file.seekg(-5,ios::cur);
            treeConstructor(file,child_tmp);
            file >> curr;
        }
        
        // reaching the end node
        if(curr == "End"){
            joint *endChild = new joint;
            endChild->name = root->name;
            endChild->name += "_End";
            root->children.push_back(endChild);
            file.seekg(-3,ios::cur);
            treeConstructor(file, endChild);
            file >> curr;
        }
        
    }else if(curr == "End"){
        
        root->joint_type = curr;
        
        // load offsets
        do{
            file >> curr;
        }while(curr != "OFFSET");
        file >> root->offset_x >> root->offset_y >> root->offset_z;
        
        file >> curr;
        //cout << "returing from End Site" << endl;
    }
}

void setMeta(ifstream &file, META &meta_data){
    string curr;
    file >> curr;cout << curr << " ";
    file >> meta_data.frame;
    cout << meta_data.frame << endl;
    file >> curr;cout << curr << " ";file >> curr;cout << curr << " ";
    file >> meta_data.frame_time;
    cout << meta_data.frame_time << endl;
}

void parseMotion(ifstream &file,joint* &node){
    // end case
    if(node->joint_type == "End"){
        vector<double> emptyVec;
        node->motion.push_back(emptyVec);
        return;
    }
    
    // update motion data for the current node
    double posi;
    int numberOfChannels = node->channels.size();
    vector<double> tmp;
    for(int i=0;i<numberOfChannels;i++){
        file >> posi;
        tmp.push_back(posi);
    }
    node->motion.push_back(tmp);
    
    // update motion data for all children
    for(auto child: node->children){
        parseMotion(file,child);
    }
    
    if(node->name == "hip") counts++;
    
}

void outDFS(ofstream &ofile,joint *root);

void jsonify(joint root,META meta_data){
    cout << "Start jsonifying…" << endl;
    
    ofstream ofile("output_fin.json");
    if(!ofile.is_open()){
        cout << "Error creating output.json, exiting…" << endl;
        exit(0);
    }
    
    // output meta data
    ofile << "{" << endl << "\"frame\": " << meta_data.frame << "," << endl
    << "\"frame_time\": " << meta_data.frame_time << "," << endl;
    
    // output the tree
    ofile << "\"joint\":" << endl;
    joint *rootptr = &root;
    outDFS(ofile,rootptr);
    
    // output final closing bracket
    ofile << "}";
    
    cout << "Jsonify completed" << endl;
}

void outDFS(ofstream &ofile,joint *root){
    
    // output basic info
    ofile << "{" << endl;
    ofile << "\"type\": " << "\"" << root->joint_type << "\"," << endl
    << "\"name\": " << "\"" << root->name << "\"," << endl
    << "\"offset\": " << "[" << root->offset_x << ", " << root->offset_y << ", " << root->offset_z << "]," << endl;
    
    if(root->joint_type == "End"){
        
        // output channel data for end nodes
        ofile << "\"channels\": []," << endl;
        
        // output motion data for end nodes
        ofile << "\"motion\": [" << endl;
        for(auto line: root->motion){
            ofile << "[]," << endl;
        }
        ofile.seekp(-2,ios::cur);
        ofile << endl << "]," << endl;
        
    }else{
        
        // output channel data for non-end nodes
        ofile << "\"channels\": [";
        for(auto channel: root->channels){
            ofile << "\"" << channel << "\", ";
        }
        ofile.seekp(-2,ios::cur);
        ofile.write("],",2);
        ofile << endl;
        
        // output motion data for non-end nodes
        ofile << "\"motion\": [" << endl;
        for(auto line: root->motion){
            ofile << "[";
            for(auto it: line){
                ofile << it << ", ";
            }
            ofile.seekp(-2,ios::cur);
            ofile.write("],",2);
            ofile << endl;
        }
        ofile.seekp(-2,ios::cur);
        ofile << endl;
        ofile << "]," << endl;
    }
    
    // output children data
    ofile << "\"children\": [ " << endl;
    for(auto child: root->children){
        outDFS(ofile, child);
        ofile << ",";
    }
    ofile.seekp(-1,ios::cur);
    ofile << endl;
    ofile << "]" << endl;
    
    // close the bracket
    ofile << "}" << endl;
    
}

void clearMotionData(joint *root){
    root->motion.pop_back();
    for(auto child: root->children){
        clearMotionData(child);
    }
}

int main(int argc, char** argv) {
    joint root;
    joint *root_ptr = new joint;
    META meta_data;
    ifstream file("sample.bvh");
    if(!file.is_open()){
        cout << "not opened" << endl; exit(0);
    }else{
        cout << "Start parsing…" << endl;
    }
    

    while (file.good()) {
        string curr;
        file >> curr;
        if(curr == "HIERARCHY"){
            cout << "Tree constructing…" << endl;
            treeConstructor(file,root_ptr);
        }else if(curr == "MOTION"){
            cout << "Tree construction complete, parsing motion data…" << endl << endl;
            setMeta(file,meta_data);
            cout << "META set…" << endl;
            break;
        }
    }
    
    while (file.good()) {
        parseMotion(file,root_ptr);
        //cout << file.tellg() << endl;
    }
    
    clearMotionData(root_ptr);  // delete one extra motion *?*
    
    cout << "Motion data parsing completed…" << endl;
    
    root = *root_ptr;
    cout << "bvh file parsing completed." << endl << endl;
    
    jsonify(root, meta_data);
    file.close();
    return 0;
}

