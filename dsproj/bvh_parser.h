//
//  bvh_parser.h
//  dsproj
//
//  Created by 谢文昊 on 2022/12/9.
//

#ifndef bvh_parser_h
#define bvh_parser_h
#include<vector>

using std::string;
using std::vector;

struct joint {
    string name;
    double offset_x, offset_y, offset_z;
    vector<joint*> children;
    vector<string> channels;
    vector<vector<double>> motion;
    string joint_type;
};

struct META {
    int frame;
    double frame_time;
};

void jsonify (joint, META);


#endif /* bvh_parser_h */
