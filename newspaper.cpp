//
//  main.cpp
//  imageSeg
//
//  Created by Panfeng Cao on 16/6/14.
//  Copyright © 2016年 Panfeng Cao. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include "tinyxml2.cpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace std;
using namespace cv;

struct rect{
    int left;
    int right;
    int top;
    int bottom;
};

struct pattern{
    vector<rect> rects;
    float bp_density;
    float vbrl;
    float sp;
    int mbrl;
    int nbr;
    int left;
    int right;
    int top;
    int bottom;
    int width;
    int height;
    int area;
    int num_bp;
    int type;
};

struct block{
    vector<pattern> patterns;
    int left;
    int right;
    int top;
    int bottom;
    int width;
    int height;
    int area;
    int type;
};

//6.22 NOTE: file name
Mat image_ori0 = imread("0033.jpg", 0);
Mat image_ori;
Mat image;
Mat imageBGR;
int height;
int width;

int search_right(int x, int y, int xstep){
    for(int j = y;j<=min(y+2, height);j++){
        for(int i = min(x+2, width);i >= x + 3 - xstep; i--){
            if(!image.at<uchar>(j-1, i-1)){
                return i-x+1;
            }
        }
    }
    return 0;
}

int lowest_row(rect R){
    for(int i = min(R.top+2, height);i>=R.top;i--){
        for(int j = R.left;j<=min(R.right, width);j++)
            if(!image.at<uchar>(i-1, j-1))
                return i - R.top + 1;
    }
    return 0;
}

bool checksquare(int i, int j){
//    sum(sum(img(i:i+2,j:j+2).*ones(3,3))) == 9
    if(image.at<uchar>(i, j) && image.at<uchar>(i+1, j) && image.at<uchar>(i+2, j) && image.at<uchar>(i, j+1) && image.at<uchar>(i, j+2) && image.at<uchar>(i+1, j+1) && image.at<uchar>(i+1, j+2) && image.at<uchar>(i+2, j+1) && image.at<uchar>(i+2, j+2))
        return true;
    else
        return false;
}

int search_below(rect R, int ry, int ystep){
    for(int i = ry + 2;i>=ry+3-ystep;i--){
        for(int j = R.left;j<=min(R.right, width)-2;j++)
            if(i+2>height || checksquare(i-1, j-1))
                return 0;
    }
    R.top = ry;
    return lowest_row(R);
}

vector<rect> locate_rects(){
    int x = 1, y = 1, xstep = 3;
    int ystep, ry;
    vector<rect> ret;
    while(y <= height && x <= width){
        if(xstep == search_right(x, y, xstep)){
            rect tmp;
            tmp.left = x;
            tmp.top = y;
            x = x+xstep;
            while(xstep == search_right(x, y, xstep))
                x = x+xstep;
            tmp.right = min(x, width);
            ystep = lowest_row(tmp);
            ry = y + ystep;
            while(ystep == search_below(tmp, ry, ystep))
                ry += ystep;
            tmp.bottom = min(ry, height);
            ret.push_back(tmp);
        }
        else
            x += 3;
        if(x >= width){
            x = 1;
            y += 3;
        }
    }
    cout<<"The total number of rects is: "<<ret.size()<<endl;
    cout<<"The estimated time for generating the patterns is: "<<(float)ret.size()/650<<"s"<<endl;
    return ret;
}

bool check_near(rect rectA, vector<rect> rects, int threshold){
    long num_rects = rects.size();
    int width, height;
    for(int i = 0;i < num_rects;i++){
        width = max(rectA.right, rects[i].right)-min(rectA.left, rects[i].left);
        height = max(rectA.bottom, rects[i].bottom)-min(rectA.top, rects[i].top);
        if(width <= rectA.right-rectA.left+rects[i].right-rects[i].left+threshold && height <= rectA.bottom-rectA.top+rects[i].bottom-rects[i].top+threshold)
            return true;
    }
    return false;
}

bool check_tt_near(block A, block B, float hgap, float vgap){
    float width = (float)max(A.right, B.right)-min(A.left, B.left);
    float height = (float)max(A.bottom, B.bottom)-min(A.top, B.top);
    if(width <= A.right-A.left+B.right-B.left+hgap || height <= A.bottom-A.top+B.bottom-B.top+vgap)
        return true;
    return false;
}

vector<pattern> generate_patterns(vector<rect> rects){
    vector<pattern> ret;
    pattern tmp;
    long num_patterns;
    long num_rects = rects.size();
    tmp.rects.push_back(rects[0]);
    ret.push_back(tmp);
    vector<int> todel;
    int rects_count = 1;
    while(rects_count < num_rects){
        num_patterns = ret.size();
        for(int i = 1;i<=num_patterns;i++){
            if(check_near(rects[rects_count], ret[i-1].rects, 1))
                todel.push_back(i - 1);
        }
        if(todel.size() == 1)
            ret[todel.front()].rects.push_back(rects[rects_count]);
        else if(todel.size() > 1){
            pattern tmp;
            tmp.rects.push_back(rects[rects_count]);
            for(auto i = todel.rbegin();i!=todel.rend();i++){
                for(auto j:ret[*i].rects)
                    tmp.rects.push_back(j);
                ret.erase(ret.begin() + *i);
            }
            ret.push_back(tmp);
        }
        else{
            pattern tmp;
            tmp.rects.push_back(rects[rects_count]);
            ret.push_back(tmp);
        }
        rects_count++;
        cout<<rects_count<<endl;
        todel.clear();
    }
    cout<<"The total number of patterns is "<<ret.size()<<endl;
    cout<<"The estimated time for generating the blocks is "<<(float)ret.size()/250<<"s"<<endl;
    return ret;
}

void mbprl(int left, int right, int top, int bottom, int& mbrl, int& nbr, float& vbrl){
    int start, p;
    int len;
    int tmp_width = right-left+1;
    int tmp_height = bottom-top+1;
    vector<vector<int>> length(tmp_height, vector<int>(tmp_width, 0));
//    int length[tmp_height][tmp_width];
    for(int i = top;i <= bottom;i++){
        start = left;
        while(start <= right){
            if(!image.at<uchar>(i-1, start-1)){
                len = 1;
                p = start + 1;
                if(p <= right){
                    while(!image.at<uchar>(i-1, p-1)){
                        len++;
                        p++;
                        if(p > right)
                            break;
                    }
                }
                length[i-top][start-left] = len;
                start += len;
            }else
                start++;
        }
    }
    
    mbrl = INT_MIN;
    nbr = 0;
    vector<int> calcstd;
    float summation = 0;
    float sumtmp = 0;
    for(int i = 0;i < tmp_height;i++){
        for(int j = 0;j< tmp_width;j++){
            int tmp = length[i][j];
            if(tmp > mbrl)
                mbrl = tmp;
            if(tmp != 0){
                nbr++;
                summation += tmp;
                calcstd.push_back(tmp);
            }
        }
    }
    long num_tmp = calcstd.size();
    summation /= num_tmp;
    for(auto i:calcstd)
        sumtmp += pow((i - summation), 2);
    vbrl = sqrt(sumtmp/num_tmp);
}

void get_pattern_para(vector<pattern> &patterns, bool line){
    long num_patterns = patterns.size();
    for(int i = 0;i<num_patterns;i++){
        patterns[i].left = INT_MAX;
        patterns[i].top = INT_MAX;
        patterns[i].right = INT_MIN;
        patterns[i].bottom = INT_MIN;
        for(auto j:patterns[i].rects){
            if(j.left < patterns[i].left)
                patterns[i].left = j.left;
            if(j.top < patterns[i].top)
                patterns[i].top = j.top;
            if(j.right > patterns[i].right)
                patterns[i].right = j.right;
            if(j.bottom > patterns[i].bottom)
                patterns[i].bottom = j.bottom;
        }
        patterns[i].right = min(patterns[i].right, width);
        patterns[i].bottom = min(patterns[i].bottom, height);
        patterns[i].width = patterns[i].right - patterns[i].left;
        patterns[i].height = patterns[i].bottom - patterns[i].top;
        patterns[i].area = patterns[i].height * patterns[i].width;
        if(!line){
            patterns[i].num_bp = 0;
            for(int k1 = patterns[i].top;k1<=patterns[i].bottom;k1++)
                for(int k2 = patterns[i].left;k2<=patterns[i].right;k2++)
                    if(!image.at<uchar>(k1-1, k2-1))
                        patterns[i].num_bp++;
            patterns[i].bp_density = float(patterns[i].num_bp)/(patterns[i].area - patterns[i].num_bp);
            mbprl(patterns[i].left, patterns[i].right, patterns[i].top, patterns[i].bottom, patterns[i].mbrl, patterns[i].nbr, patterns[i].vbrl);
            patterns[i].sp = (float(patterns[i].nbr)/patterns[i].num_bp)*(pow(min(patterns[i].width, patterns[i].height), 2));
        }
    }
}

void pattern_classfy(vector<pattern>& patterns, vector<pattern>& text, vector<pattern>& title){
    get_pattern_para(patterns, false);
    //estimate he
    vector<int> index;
    float he = 0;
    for(int i = 0;i<patterns.size();i++){
        if(patterns[i].height<25){
            index.push_back(i);
            he += patterns[i].height;
        }
    }
    he = he/index.size();
    
//    cout<<"he: "<<he<<endl;
    
    //parameter definition
    //DD                            DI                      Ratios
    float A1 = 400*he*he;      float A2 = 32;         float alpha = 0.75;
    float A3 = 80*he*he;       float B = 16;          float beta = 0.04;
    float A4 = 64*he*he;       float S1 = 350;        float kapa = 1.4;
    float D = 5*he;            float S2 = 500;        float lambda1 = 0.16;
    float H = 0.7*he;          float V = 5.5;         float lambda2 = 0.1;
    float L = 3.0*he;                                 float lambda3 = 0.13;
    float W1 = 1.5*he;                                float rho0 = 0.3;
    float W2 = 10*he;                                 float rho1 = 0.72;
                                                      float rho2 = 0.5;
                                                      float rho3 = 1.65;
    //type 1:1 Text 2 Title 3 Inverse text 4 Photograph 5 Graphic/drawing 6 Vertical line 7 Horizontal line 8 Small
    
    //Rule A Large Patterns
    vector<int> index_large_patterns;
    for(int i = 0;i<patterns.size();i++){
        if(patterns[i].area > A1 && patterns[i].width>D && patterns[i].height>D)
            index_large_patterns.push_back(i);
    }
//    cout<<"size: "<<index_large_patterns.size()<<endl;
//    for(auto i:index_large_patterns)
//        cout<<"index_large_patterns: "<<i<<endl;
    
    //Rule B Photograph or Graphic
    vector<int> index_photograph;
    vector<int> index_graph;
    for(int i = 0;i<index_large_patterns.size();i++)                        //added constraint besides paper
        if(patterns[index_large_patterns[i]].bp_density > rho0 && patterns[index_large_patterns[i]].num_bp > 30000){
            patterns[index_large_patterns[i]].type = 4;
            index_photograph.push_back(index_large_patterns[i]);
        }
        else{
            patterns[index_large_patterns[i]].type = 5;
            index_graph.push_back(index_large_patterns[i]);
        }
//    cout<<"size: "<<index_photograph.size()<<endl;
//    for(auto i:index_photograph)
//        cout<<"index_photograph: "<<i<<endl;
//    cout<<"size: "<<index_graph.size()<<endl;
//    for(auto i:index_graph)
//        cout<<"index_graph: "<<i<<endl;
    
    //Rule C Small Patterns
    index.clear();
    for(int i = 0;i<patterns.size();i++)
        index.push_back(i);
    for(auto i = index_large_patterns.rbegin();i!=index_large_patterns.rend();i++)
        index.erase(index.begin() + *i);
    
    vector<int> index_small_patterns;
    vector<int> todel;
    for(int i = 0;i<index.size();i++)
        if(patterns[index[i]].num_bp < B && patterns[index[i]].area < A2){
            patterns[index[i]].type = 8;
            index_small_patterns.push_back(index[i]);
            todel.push_back(i);
        }
    
//    cout<<"size: "<<index_small_patterns.size()<<endl;
//    for(auto i:index_small_patterns)
//        cout<<"index_small_patterns: "<<i<<endl;
    
    //Rule D Vertical Lines
    vector<int> index_vertical_lines;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float h = (float)patterns[index[i]].height;
        if(h > L && patterns[index[i]].width < min(lambda1*h, W1)){
            patterns[index[i]].type = 6;
            index_vertical_lines.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_vertical_lines.size()<<endl;
//    for(auto i:index_vertical_lines)
//        cout<<"index_vertical_lines: "<<i<<endl;
    
    //Rule E Vertical long thick lines
    vector<int> index_vlt_lines;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float w = (float)patterns[index[i]].width;
        float h = (float)patterns[index[i]].height;
        if(w >= W1 && w < lambda2*h){
            patterns[index[i]].type = 6;
            index_vlt_lines.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_vlt_lines.size()<<endl;
//    for(auto i:index_vlt_lines)
//        cout<<"index_vlt_lines: "<<i<<endl;
    
    //Rule F Horizontal Lines
    vector<int> index_horizontal_lines;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float w = (float)patterns[index[i]].width;
        float h = (float)patterns[index[i]].height;
        float d = patterns[index[i]].bp_density;
        float m = patterns[index[i]].mbrl;
        if(w>L && h<min(lambda1*w, W1)){
            if(h < H || h < lambda3*w || m > 2*h || d > rho1){
                patterns[index[i]].type = 7;
                index_horizontal_lines.push_back(index[i]);
                todel.push_back(i);
            }
        }
    }
//    cout<<"size: "<<index_horizontal_lines.size()<<endl;
//    for(auto i:index_horizontal_lines)
//        cout<<"index_horizontal_lines: "<<i+1<<endl;
    
    //Rule G Thick Horizontal Lines
    vector<int> index_th_lines;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float w = (float)patterns[index[i]].width;
        float h = (float)patterns[index[i]].height;
        if(h >= W1 && h < lambda2*w){
            patterns[index[i]].type = 7;
            index_th_lines.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_th_lines.size()<<endl;
//    for(auto i:index_th_lines)
//        cout<<"index_th_lines: "<<i<<endl;
    
    //Rule H Photographs
    vector<int> index_ps;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float a = (float)patterns[index[i]].area;
        float n = (float)patterns[index[i]].nbr;
        if(a > A3 && n > beta*a){
            patterns[index[i]].type = 4;
            index_ps.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_ps.size()<<endl;
//    for(auto i:index_ps)
//        cout<<"index_ps: "<<i<<endl;
    
    //Rule I Graphics
    vector<int> index_gs;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float a = (float)patterns[index[i]].area;
        float s = (float)patterns[index[i]].sp;
        float v = (float)patterns[index[i]].vbrl;
        if(a < A4 && s > S1 && v > V){
            patterns[index[i]].type = 5;
            index_gs.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_gs.size()<<endl;
//    for(auto i:index_gs)
//        cout<<"index_gs: "<<i<<endl;
    
    //Rule J Graphics in larger patterns
    vector<int> index_gslp;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float a = (float)patterns[index[i]].area;
        float s = (float)patterns[index[i]].sp;
        float d = (float)patterns[index[i]].bp_density;
        if(a >= A4 && s > S2 && d < rho2){
            patterns[index[i]].type = 5;
            index_gslp.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_gslp.size()<<endl;
//    for(auto i:index_gslp)
//        cout<<"index_gslp: "<<i<<endl;
    
    //Rule K Inverse Text
    vector<int> index_it;
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    todel.clear();
    
    for(int i = 0;i<index.size();i++){
        float w = (float)patterns[index[i]].width;
        float m = (float)patterns[index[i]].mbrl;
        float d = (float)patterns[index[i]].bp_density;
        if(w > W2 && d > rho3 && m > alpha*w){
            patterns[index[i]].type = 3;
            index_it.push_back(index[i]);
            todel.push_back(i);
        }
    }
//    cout<<"size: "<<index_it.size()<<endl;
//    for(auto i:index_it)
//        cout<<"index_it: "<<i<<endl;
    
    //recalculate ht
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    
    float ht = 0;
    for(int i = 0;i<index.size();i++)
        ht = ht + patterns[index[i]].height;
    
    ht = ht/index.size();
    
    //Rule L Text or Title
    vector<int> index_tt;
    todel.clear();

    for(int i = 0;i<index.size();i++){
        float h = (float)patterns[index[i]].height;
        float w = (float)patterns[index[i]].width;
    //     if h > kapa*ht && patterns[index[i-1]].bp_density > 0.5 && w > 20
        if(h > kapa*ht && patterns[index[i]].bp_density > 0.5 && w > 10){
            patterns[index[i]].type = 2;
            title.push_back(patterns[index[i]]);
            index_tt.push_back(index[i]);
            todel.push_back(i);
        }
        }
    
    //the remains are all texts
    //index works for texts now
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        index.erase(index.begin() + *i);
    for(int i = 0;i<index.size();i++){
        patterns[index[i]].type = 1;
        text.push_back(patterns[index[i]]);
    }
}

vector<pattern> generate_boxes(vector<pattern>& patterns){
    vector<pattern> boxes;
    long num_patterns = patterns.size();
    float ht = 11.848122162608338;//optimize*************************************
    float W1 = 12.2750;
    float delta;
    vector<int> todel;
    bool flag;
    int tmp;
    int w, h, top, left, right, bottom, num_rects;
    
    for(int i = 0;i<num_patterns;i++){
        w = patterns[i].width;
        h = patterns[i].height;
        top = patterns[i].top;
        left = patterns[i].left;
        right = patterns[i].right;
        bottom = patterns[i].bottom;
        num_rects = (int)patterns[i].rects.size();
    
        if(max(w, h) <= 3*ht)
            continue;
        
        delta = min(W1, float(min(w, h))/4);
        flag = true;
    
        for(int j = 0;j<num_rects;j++){
            tmp = min(min(patterns[i].rects[j].bottom - top, bottom - patterns[i].rects[j].top), min(right - patterns[i].rects[j].left, patterns[i].rects[j].right - left));
            if(tmp > delta){
                flag = false;
                break;
            }
        }
    
        if(flag){
            boxes.push_back(patterns[i]);
            todel.push_back(i);
        }
    }
    
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        patterns.erase(patterns.begin() + *i);
    
    return boxes;
}

vector<pattern> extract_line_boxes(vector<pattern>& boxes, int count_threshold){
    long num_boxes = boxes.size();
    vector<pattern> line_patterns;
    int num_rects, w, h, top, left ,right, bottom;
    int tmp;
    pattern top_line;
    pattern left_line;
    pattern bottom_line;
    pattern right_line;
    top_line.type = 7;
    bottom_line.type = 7;
    left_line.type = 6;
    right_line.type = 6;
    
    for(int i = 0;i<num_boxes;i++){
        num_rects = (int)boxes[i].rects.size();
        w = boxes[i].width;
        h = boxes[i].height;
        top = boxes[i].top;
        left = boxes[i].left;
        right = boxes[i].right;
        bottom = boxes[i].bottom;
    
        for(int j = 0;j < num_rects;j++){
            tmp = min(min(boxes[i].rects[j].bottom - top, bottom - boxes[i].rects[j].top), min(boxes[i].rects[j].right - left, right - boxes[i].rects[j].left));
            if(boxes[i].rects[j].bottom - top == tmp)
                top_line.rects.push_back(boxes[i].rects[j]);
            if(bottom - boxes[i].rects[j].top == tmp)
                bottom_line.rects.push_back(boxes[i].rects[j]);
            if(boxes[i].rects[j].right - left == tmp)
                left_line.rects.push_back(boxes[i].rects[j]);
            if(right - boxes[i].rects[j].left == tmp)
                right_line.rects.push_back(boxes[i].rects[j]);
            }
        
        if(top_line.rects.size() >= count_threshold)
            line_patterns.push_back(top_line);
        if(left_line.rects.size() >= count_threshold)
            line_patterns.push_back(left_line);
        if(bottom_line.rects.size() >= count_threshold)
            line_patterns.push_back(bottom_line);
        if(right_line.rects.size() >= count_threshold)
            line_patterns.push_back(right_line);
        
        top_line.rects.clear();
        left_line.rects.clear();
        bottom_line.rects.clear();
        right_line.rects.clear();
    }
    get_pattern_para(line_patterns, true);
    return line_patterns;
}

vector<pattern> reclass(vector<pattern>& patterns, vector<pattern>& line_patterns){
    long num_lines = line_patterns.size();
    vector<rect> Rects;
    for(auto im:patterns)
        for(auto jq:im.rects)
            Rects.push_back(jq);
    
//    int sum = 0;
//        for(auto j:patterns){
//            cout<<j.rects.size()<<endl;
//            sum+=j.rects.size();
//        }
//    cout<<sum<<endl;
    
    vector<bool> flag = vector<bool>(num_lines, false);
    vector<pattern> new_line;
    vector<int> todel;
    
    int factor = 2;
    int align_range[2];
    int merge_range;
    
    for(int i = 0;i < num_lines;i++){
        if(flag[i])
            continue;
        
        flag[i] = true;
    
        switch(line_patterns[i].type){
            case 6: {//vertical lines
                align_range[0] = line_patterns[i].left - line_patterns[i].width;
                align_range[1] = line_patterns[i].right + line_patterns[i].width;
                merge_range = factor*line_patterns[i].width;
                for(int j = i+1;j < num_lines;j++){
                    if(flag[j])
                        continue;
                    if((min(min(abs(line_patterns[i].top - line_patterns[j].bottom), abs(line_patterns[i].top - line_patterns[j].top)), min(abs(line_patterns[i].bottom - line_patterns[j].top), abs(line_patterns[i].bottom - line_patterns[j].bottom))) < merge_range) && (line_patterns[j].left > align_range[0]) && (line_patterns[j].right < align_range[1])){
                        
                        flag[j] = true;
                        for(auto p:line_patterns[j].rects)
                            line_patterns[i].rects.push_back(p);
        
                        line_patterns[i].right = max(line_patterns[i].right, line_patterns[j].right);
                        line_patterns[i].left = min(line_patterns[i].left, line_patterns[j].left);
                        line_patterns[i].top = min(line_patterns[i].top, line_patterns[j].top);
                        line_patterns[i].bottom = max(line_patterns[i].bottom, line_patterns[j].bottom);
                        line_patterns[i].right = min(line_patterns[i].right, width);
                        line_patterns[i].bottom = min(line_patterns[i].bottom, height);
                        line_patterns[i].width = line_patterns[i].right - line_patterns[i].left;
                        line_patterns[i].height = line_patterns[i].bottom - line_patterns[i].top;
                        line_patterns[i].area = line_patterns[i].height * line_patterns[i].width;
                    }
                }
                merge_range = factor*line_patterns[i].width;
                todel.clear();
                for(int k = 0;k < Rects.size();k++){
                    if((min(min(abs(line_patterns[i].top - Rects[k].bottom), abs(line_patterns[i].top - Rects[k].top)), min(abs(line_patterns[i].bottom - Rects[k].top), abs(line_patterns[i].bottom - Rects[k].bottom))) < merge_range) && (Rects[k].left > align_range[0]) && (Rects[k].right < align_range[1])){
                        
                        todel.push_back(k);
                        line_patterns[i].rects.push_back(Rects[k]);
                
                        line_patterns[i].right = max(line_patterns[i].right, Rects[k].right);
                        line_patterns[i].left = min(line_patterns[i].left, Rects[k].left);
                        line_patterns[i].top = min(line_patterns[i].top, Rects[k].top);
                        line_patterns[i].bottom = max(line_patterns[i].bottom, Rects[k].bottom);
                        line_patterns[i].right = min(line_patterns[i].right, width);
                        line_patterns[i].bottom = min(line_patterns[i].bottom, height);
                        line_patterns[i].width = line_patterns[i].right - line_patterns[i].left;
                        line_patterns[i].height = line_patterns[i].bottom - line_patterns[i].top;
                        line_patterns[i].area = line_patterns[i].height * line_patterns[i].width;
            }
                }
            new_line.push_back(line_patterns[i]);
            for(auto q = todel.rbegin();q!=todel.rend();q++)
                Rects.erase(Rects.begin() + *q);
            break;
            }
                
            case 7: {//horizontal lines
                align_range[0] = line_patterns[i].top - line_patterns[i].height;
                align_range[1] = line_patterns[i].bottom + line_patterns[i].height;
                merge_range = factor*line_patterns[i].height;
                for(int j = i+1;j < num_lines;j++){
                    if(flag[j])
                        continue;
                    if((min(min(abs(line_patterns[i].left - line_patterns[j].left), abs(line_patterns[i].left - line_patterns[j].right)), min(abs(line_patterns[i].right - line_patterns[j].left), abs(line_patterns[i].right - line_patterns[j].right))) < merge_range) && (line_patterns[j].top > align_range[0]) && (line_patterns[j].bottom < align_range[1])){
                        
                        flag[j] = true;
                        for(auto p:line_patterns[j].rects)
                            line_patterns[i].rects.push_back(p);
                        
                        line_patterns[i].right = max(line_patterns[i].right, line_patterns[j].right);
                        line_patterns[i].left = min(line_patterns[i].left, line_patterns[j].left);
                        line_patterns[i].top = min(line_patterns[i].top, line_patterns[j].top);
                        line_patterns[i].bottom = max(line_patterns[i].bottom, line_patterns[j].bottom);
                        line_patterns[i].right = min(line_patterns[i].right, width);
                        line_patterns[i].bottom = min(line_patterns[i].bottom, height);
                        line_patterns[i].width = line_patterns[i].right - line_patterns[i].left;
                        line_patterns[i].height = line_patterns[i].bottom - line_patterns[i].top;
                        line_patterns[i].area = line_patterns[i].height * line_patterns[i].width;
                    }
                }
                
                merge_range = factor*line_patterns[i].height;
                todel.clear();
                
                for(int k = 0;k < Rects.size();k++){
                    if((min(min(abs(line_patterns[i].left - Rects[k].left), abs(line_patterns[i].left - Rects[k].right)), min(abs(line_patterns[i].right - Rects[k].left), abs(line_patterns[i].right - Rects[k].right))) < merge_range) && (Rects[k].top > align_range[0]) && (Rects[k].bottom < align_range[1])){
                        todel.push_back(k);
                        
                        line_patterns[i].rects.push_back(Rects[k]);
                        line_patterns[i].right = max(line_patterns[i].right, Rects[k].right);
                        line_patterns[i].left = min(line_patterns[i].left, Rects[k].left);
                        line_patterns[i].top = min(line_patterns[i].top, Rects[k].top);
                        line_patterns[i].bottom = max(line_patterns[i].bottom, Rects[k].bottom);
                        line_patterns[i].right = min(line_patterns[i].right, width);
                        line_patterns[i].bottom = min(line_patterns[i].bottom, height);
                        line_patterns[i].width = line_patterns[i].right - line_patterns[i].left;
                        line_patterns[i].height = line_patterns[i].bottom - line_patterns[i].top;
                        line_patterns[i].area = line_patterns[i].height * line_patterns[i].width;                    }
        }
                new_line.push_back(line_patterns[i]);
                for(auto q = todel.rbegin();q!=todel.rend();q++)
                    Rects.erase(Rects.begin() + *q);
                break;
            }
        }
    }
    //     pattern = generate_patterns(rects);
    //     pattern = [pattern; new_line];
    //     pattern = pattern_classify(pattern, img);
    return new_line;
}

void get_block_para(block& blocks){
    blocks.left = INT_MAX;
    blocks.top = INT_MAX;
    blocks.right = INT_MIN;
    blocks.bottom = INT_MIN;
    for(auto j:blocks.patterns){
        if(j.left < blocks.left)
            blocks.left = j.left;
        if(j.top < blocks.top)
            blocks.top = j.top;
        if(j.right > blocks.right)
            blocks.right = j.right;
        if(j.bottom > blocks.bottom)
            blocks.bottom = j.bottom;
    }
    blocks.right = min(blocks.right, width);
    blocks.bottom = min(blocks.bottom, height);
    blocks.width = blocks.right - blocks.left;
    blocks.height = blocks.bottom - blocks.top;
    blocks.area = blocks.height * blocks.width;
}

bool near(pattern x, pattern y, float hgap, float vgap, float factor){
    rect rectX;
    rect rectY;
    rectX.left = x.left; rectX.right = x.right; rectX.top = x.top; rectX.bottom = x.bottom;
    rectY.left = y.left; rectY.right = y.right; rectY.top = y.top; rectY.bottom = y.bottom;
    
    if((x.left < y.right + factor*hgap) && (x.right > y.left - factor*hgap))
        if((x.top < y.bottom + factor*vgap) && (x.bottom > y.top - factor*vgap))
            return true;
    
    if((x.left >= y.left) && (x.right <= y.right) && (x.top >= y.top) && (x.bottom <= y.bottom))
        return true;
    
    if((y.left >= x.left) && (y.right <= x.right) && (y.top >= x.top) && (y.bottom <= x.bottom))
        return true;
    
    vector<rect> rectsY;
    vector<rect> rectsX;
    rectsY.push_back(rectY);
    rectsX.push_back(rectX);
    
    if(check_near(rectX, rectsY, hgap) || check_near(rectY, rectsX, vgap))
        return true;
    
    return false;
}

void get_block_gap(vector<pattern>& patterns, float& avh, float& hgap, float& vgap){
    float num_texts = 0;
    long num_patterns = patterns.size();
    vector<int> tt_height;
    for(int i = 0;i<num_patterns;i++){
        if(patterns[i].type == 1 || patterns[i].type == 2){
            num_texts++;
            tt_height.push_back(patterns[i].height);
        }
    }
    sort(tt_height.begin(), tt_height.end());
    int start = ceil(num_texts/4.0);
    int End = ceil(3*num_texts/4.0);
    float summ = accumulate(tt_height.begin()+start, tt_height.begin()+End, 0);
    avh = summ/(End-start);
    hgap = 1.1*avh;
    vgap = 0.8*avh;
}

vector<block> getblocks(vector<block>& blocks, int type, vector<int>& index){
    vector<block> tmp_blk;
    index.clear();
    for(int p = 0;p < blocks.size();p++){
        if(blocks[p].type == type){
            tmp_blk.push_back(blocks[p]);
            index.push_back(p);
        }
    }
    return tmp_blk;
}

vector<block> regionform(vector<pattern>& patterns){
    long num_patterns = patterns.size();
    bool grouped;
    bool found;
    float avh, hgap, vgap;
    int initial_block = 0;
    get_block_gap(patterns, avh, hgap, vgap);
    vector<int> idx;
    vector<int> todel;
    vector<block> blocks;
    vector<block> tmp_blk;
    
    for(int i = 0;i < num_patterns;i++){
        cout<<i<<endl;
//        cout<<i+1<<" type: "<<patterns[i].type<<" left :"<<patterns[i].left<<" right :"<<patterns[i].right<<" top :"<<patterns[i].top<<" bottom :"<<patterns[i].bottom<<endl;
//        for(auto pp:blocks)
//            cout<<i+1<<":"<<"blocks size: "<<blocks.size()<<" type: "<<pp.type<<" left :"<<pp.left<<endl;
//        cout<<blocks.size()<<endl;
        grouped = false;
        
//6.21 NOTE: Blocks of different types are not mixingly processed any more.
//        tmp_blk = getblocks(blocks, patterns[i].type, idx);
        tmp_blk = blocks;
        for(int j = 0;j < tmp_blk.size();j++){
            found = false;
            for(int k = 0;k < tmp_blk[j].patterns.size();k++){
                if(near(patterns[i], tmp_blk[j].patterns[k], hgap, 2*vgap, 1)){
                    found = true;
                    break;
                }
            }
            
            if(found){
                if(!grouped){
                    tmp_blk[j].patterns.push_back(patterns[i]);
                    get_block_para(tmp_blk[j]);
                    initial_block = j;
                    grouped = true;
                }else{
                    for(auto m:tmp_blk[j].patterns)
                        tmp_blk[initial_block].patterns.push_back(m);
                    get_block_para(tmp_blk[initial_block]);
                    todel.push_back(j);
                }
            }
        }
        
        for(auto jj1 = todel.rbegin();jj1!=todel.rend();jj1++)
            tmp_blk.erase(tmp_blk.begin()+ *jj1);
        
//6.21 NOTE: The idx includes all of the blocks
//        for(auto jj2 = idx.rbegin();jj2!=idx.rend();jj2++)
//            blocks.erase(blocks.begin()+ *jj2);
        blocks.clear();
        
        for(auto jj3:tmp_blk)
            blocks.push_back(jj3);
        
        if(!grouped){
            block new_blk;
            new_blk.patterns.push_back(patterns[i]);
            new_blk.type = patterns[i].type;
            get_block_para(new_blk);
            blocks.push_back(new_blk);
        }
        todel.clear();
        tmp_blk.clear();
    }
    return blocks;
}

vector<pattern> get_line_from_patterns(vector<pattern>& patterns){
    vector<pattern> ret;
    for(int i = (int)patterns.size()-1;i>=0;i--){
        if(patterns[i].type == 6 || patterns[i].type == 7){
            ret.push_back(patterns[i]);
            patterns.erase(patterns.begin()+i);
        }
}
    return ret;
}

template <typename T1, typename T2>
bool insideblk(T1 blockA, T2 blockB, int threshold , bool strict){
    if(!strict){
        if (blockA.right<=blockB.right + threshold && blockA.left>=blockB.left - threshold && blockA.top>=blockB.top - threshold && blockA.bottom<=blockB.bottom+threshold)
            return true;
    }
    else{
        if (blockA.right < blockB.right + threshold && blockA.left > blockB.left - threshold && blockA.top > blockB.top - threshold && blockA.bottom<blockB.bottom+threshold)
            return true;
    }
    return false;
}

void cleanregion(vector<block>& blocks, int threshold, bool strict){
    long num_blocks = blocks.size();
    vector<bool> flag = vector<bool>(num_blocks, 0);
    vector<int> todel;
    bool first_time;
    
    for(int i = 0;i<num_blocks;i++){
        first_time = true;
        if(flag[i])
            continue;
        
        for(int j = 0;j<num_blocks;j++){
        
        if(flag[j] || j == i)
            continue;
    
        if(insideblk(blocks[j], blocks[i], threshold, strict)){
            flag[j] = true;
            todel.push_back(j);
        }
    
        if(insideblk(blocks[i], blocks[j], threshold, strict)){
            flag[i] = true;
            if(first_time){
                todel.push_back(i);
                first_time = false;
            }
        }
    }
}
    sort(todel.begin(), todel.end());
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        blocks.erase(blocks.begin() + *i);
}

void formetablk(vector<block>& blocks, vector<block>& titleblock, vector<block>& txtblock, vector<block>& lineblk){
    vector<int> todel;
//    vector<int> txtheight;
    vector<int> titleheight;
    for(int i = 0;i<blocks.size();i++){
         if(blocks[i].type == 1){
//             txtheight.push_back(blocks[i].height);
             txtblock.push_back(blocks[i]);
             todel.push_back(i);
         }
         else if(blocks[i].type == 2){
             titleheight.push_back(blocks[i].height);
             titleblock.push_back(blocks[i]);
             todel.push_back(i);
         }else if(blocks[i].type == 6 || blocks[i].type == 7){
             lineblk.push_back(blocks[i]);
             todel.push_back(i);
         }
    }
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        blocks.erase(blocks.begin() + *i);
    
//    long num_txt = txtheight.size();
    long num_title = titleheight.size();
//    sort(txtheight.begin(), txtheight.end());
//    float summ1 = accumulate(txtheight.begin()+ceil(num_txt/4.0), txtheight.begin()+ceil(3*num_txt/4.0), 0);
    sort(titleheight.begin(), titleheight.end());
    int start = ceil(num_title/4.0);
    int End = ceil(3*num_title/4.0);
//  take the middle half of the sorted height value
    float summ2 = accumulate(titleheight.begin()+start, titleheight.begin()+End, 0);
//    float txt_avh = 2.0*summ1/num_txt;
    float title_avh = summ2/(End-start);
    
//    float txt_hgap = 1.1*txt_avh;
//    float txt_vgap = 0.8*txt_avh;
    float title_hgap = 1.1*title_avh;
    float title_vgap = 0.8*title_avh;
    
    todel.clear();
    for(int i = 0;i < txtblock.size();i++){
        if(txtblock[i].area < 1000){
            txtblock[i].type = 2;
            titleblock.push_back(txtblock[i]);
            todel.push_back(i);
        }
    }
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        txtblock.erase(txtblock.begin() + *i);
    todel.clear();
    
    int count;
    for(int i = 0;i<num_title;i++){
        count = 0;
        for(int j = 0;j<num_title;j++){
        if(j == i)
            continue;
        if(check_tt_near(titleblock[i], titleblock[j], title_hgap, title_vgap))
            count++;
        }
        if(!count){
            titleblock[i].type = 1;
            txtblock.push_back(titleblock[i]);
            todel.push_back(i);
        }
    }
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        titleblock.erase(titleblock.begin() + *i);
}

float get_BlockH(vector<pattern> patterns){
    long num_patterns = patterns.size();
    vector<int> height;
    for(int i = 0;i<num_patterns;i++)
        height.push_back(patterns[i].height);
    long num_heights = height.size();
    sort(height.begin(), height.end());
    int start = ceil(num_heights/4.0);
    int End = ceil(3*num_heights/4.0);
    float summ = accumulate(height.begin()+start, height.begin()+End, 0);
    return summ/(End-start);
}

void merge_titles(vector<block>& blocks){
    long num_titles = blocks.size();
    vector<bool> flag = vector<bool>(num_titles, false);
    float IWF = 1.05;
    float ILF = 0.75;
    vector<int> todel;
    vector<pattern> patternj;
    vector<pattern> patterni;
    float hA;
    float hB;
    float hgap;
    float vgap;
    bool break_flag;
    
    for(int i = 0;i<num_titles;i++){
        if(flag[i])
            continue;
        hA = get_BlockH(blocks[i].patterns);
        patterni = blocks[i].patterns;
    
        for(int j = i+1;j<num_titles;j++){
            if (flag[j])
                continue;
            hB = get_BlockH(blocks[j].patterns);
            hgap = 20*IWF * min(hA, hB) * min(hA, hB)/max(hA, hB);
            vgap = 1.5*ILF * min(hA, hB) * min(hA, hB)/max(hA, hB);
            patternj = blocks[j].patterns;
            break_flag = false;
        
            for(int m = 0;m<patterni.size();m++){
                for(int n = 0;n<patternj.size();n++){
                    if(abs(patterni[m].right - patternj[n].left) <= hgap || abs(patterni[m].left - patternj[n].right) <= hgap || abs(patterni[m].left - patternj[n].right) <= 10){
                   if(abs(patterni[m].bottom - patternj[n].top) <= vgap || abs(patterni[m].top - patternj[n].bottom) <= vgap || abs(patterni[m].top - patternj[n].bottom) <= 10){
                        for(auto ii:blocks[j].patterns)
                            blocks[i].patterns.push_back(ii);
                        get_block_para(blocks[i]);
                        flag[j] = true;
                        flag[i] = true;
                        break_flag = true;
                        todel.push_back(j);
                        break;
                    }
                }
                   if(break_flag)
                        break;
            }
                if(break_flag)
                    break;
            }
        }
    }
//    for(auto i:todel)
//        cout<<i<<endl;
    sort(todel.begin(), todel.end());
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        blocks.erase(blocks.begin() + *i);
}

void dealwithblocks(vector<block>& blocks, vector<pattern>& lines){
    vector<block> txtblk;
    vector<block> titleblk;
    vector<block> lineblk;
    formetablk(blocks, titleblk, txtblk, lineblk);//find line blocks
    merge_titles(titleblk);
    cleanregion(titleblk, 15, 0);
    
    for(int i = 0;i<10;i++)
        merge_titles(titleblk);
    vector<block> blk;
    for(auto i:titleblk)
        blk.push_back(i);
    for(auto j:txtblk)
        blk.push_back(j);
    
    cleanregion(blk, 15, 0);
    txtblk.clear();
    titleblk.clear();
    for(int i = 0;i<blk.size();i++){
        if(blk[i].type == 2){
            if(blk[i].patterns.size() < 5){
                blk[i].type = 1;
                txtblk.push_back(blk[i]);
            }else
                titleblk.push_back(blk[i]);
        }
        else if(blk[i].type == 1){
            txtblk.push_back(blk[i]);
        }
    }
    vector<int> todel;
    rect A;
    rect B;
    vector<rect> rectsB;
    int count;
    for(int i = 0;i<txtblk.size();i++){
        count = 0;
        for(int j = 0;j<titleblk.size();j++){
            A.left = txtblk[i].left;
            B.left = titleblk[j].left;
            A.right = txtblk[i].right;
            B.right = titleblk[j].right;
            A.top = txtblk[i].top;
            B.top = titleblk[j].top;
            A.bottom = txtblk[i].bottom;
            B.bottom = titleblk[j].bottom;
            rectsB.push_back(B);
            if(check_near(A, rectsB, 20))
                count++;
            rectsB.clear();
        }
        if(count >= 2){
            txtblk[i].type = 2;
            titleblk.push_back(txtblk[i]);
            todel.push_back(i);
        }
    }
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        txtblk.erase(txtblk.begin() + *i);
    
    for(int i = 0;i<5;i++){
        merge_titles(titleblk);
    }
    
//text blocks and title blocks are included in the total blocks
//the type of lines is pattern
    todel.clear();
    for(auto i:titleblk)
        blocks.push_back(i);
    for(auto j:txtblk)
        blocks.push_back(j);
//optimize in the future here***************************
//6.21 NOTE: don't use a total block parameter
    cleanregion(blocks, 15, 0);
    //adding lines
    for(int i = 0;i<blocks.size();i++){
        if(blocks[i].width>10*blocks[i].height && blocks[i].height < 10 && blocks[i].width > 50){
            blocks[i].type = 7;
            pattern new_line;
            new_line.type = blocks[i].type;
            new_line.right = blocks[i].right;
            new_line.left = blocks[i].left;
            new_line.top = blocks[i].top;
            new_line.bottom = blocks[i].bottom;
            new_line.width = blocks[i].width;
            new_line.height = blocks[i].height;
            lines.push_back(new_line);
            todel.push_back(i);
        }
        else if(blocks[i].height>10*blocks[i].width && blocks[i].width < 10 && blocks[i].height > 50){
            blocks[i].type = 6;
            pattern new_line;
            new_line.type = blocks[i].type;
            new_line.right = blocks[i].right;
            new_line.left = blocks[i].left;
            new_line.top = blocks[i].top;
            new_line.bottom = blocks[i].bottom;
            new_line.width = blocks[i].width;
            new_line.height = blocks[i].height;
            lines.push_back(new_line);
            todel.push_back(i);
        }
    }
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        blocks.erase(blocks.begin() + *i);
    
//clean line in text
    todel.clear();
    for(auto i:lineblk){
        pattern new_line;
        new_line.type = i.type;
        new_line.height = i.height;
        new_line.left = i.left;
        new_line.right = i.right;
        new_line.top = i.top;
        new_line.bottom = i.bottom;
        new_line.area = i.area;
        new_line.width = i.width;
        lines.push_back(new_line);
    }
    
    txtblk.clear();
    titleblk.clear();
    for(auto i:blocks)
    {
        if(i.type == 1)
            txtblk.push_back(i);
        else if(i.type == 2)
            titleblk.push_back(i);
    }
    
    bool first_time;
    for(int i = 0;i < lines.size();i++){
        first_time = true;
        for(auto j:txtblk){
            if(insideblk(lines[i], j, 0, 1)){
                todel.push_back(i);
                first_time = false;
                break;
            }
        }
        if(first_time){
            for(auto j:titleblk){
                if(insideblk(lines[i], j, 5, 1) && lines[i].width < 50 && lines[i].height < 50){
                    todel.push_back(i);
                    break;
                }
            }
        }
    }
    
    for(auto i = todel.rbegin();i!=todel.rend();i++)
        lines.erase(lines.begin()+ *i);
}

template<typename T>
void Draw(vector<T> patterns, bool print_info, string name){
    if(print_info){
        int count = 1;
        for(auto j:patterns){
            cout<<"size:"<<j.patterns.size()<<endl;
            cout<<count++<<":"<<endl;
            for(auto i:j.patterns){
                cout<<"left:"<<i.left<<" top:"<<i.top<<" right:"<<i.right<<" bottom:"<<i.bottom<<" "<<i.type<<endl;
                cout<<endl;
            }
        }
    }
    for(auto j:patterns){
        
        if(j.type != 8){
            switch(j.type){
                case 1:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(0, 0, 255), 10);
                    break;
                }
                case 2:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(0, 255, 0), 10);
                    break;
                }
                case 3:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(255, 0, 0), 10);
                    break;
                }
                case 4:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(255, 128, 128), 10);
                    break;
                }
                case 5:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(128, 128, 0), 10);
                    break;
                }
                case 6:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(0, 0, 0), 10);
                    break;
                }
                case 7:{
                    rectangle(imageBGR, Point(j.left, j.top), Point(j.right, j.bottom), CV_RGB(0, 0, 0), 10);
                    break;
                }
            }
        }
    }
    namedWindow("window", CV_WINDOW_FREERATIO);
    resizeWindow("window", 1366, 768);
    imshow("window", imageBGR);
    imwrite(name+".jpg", imageBGR);
    waitKey(0);
    destroyWindow("window");
}

template<typename T>
void saveInfo(vector<T> patterns, string name){
    ofstream file(name);
    for(auto i:patterns)
        file<<i.left<<" "<<i.right<<" "<<i.top<<" "<<i.bottom<<" "<<i.type<<" "<<i.height<<" "<<i.width<<"\n";
    file.close();
}

template<typename T>
void readInfo(vector<T>& patterns, string name){
    string line;
    ifstream file(name);
    int count;
    while(getline(file, line, '\n')){
        stringstream tmp(line);
        string tmpln;
        pattern tmpblk;
        count = 0;
        while(getline(tmp, tmpln, ' ')){
            count++;
            switch(count){
                case 1:
                    tmpblk.left = stoi(tmpln);
                    break;
                case 2:
                    tmpblk.right = stoi(tmpln);
                    break;
                case 3:
                    tmpblk.top = stoi(tmpln);
                    break;
                case 4:
                    tmpblk.bottom = stoi(tmpln);
                    break;
                case 5:
                    tmpblk.type = stoi(tmpln);
                    break;
                case 6:
                    tmpblk.height = stoi(tmpln);
                    break;
                case 7:
                    tmpblk.width = stoi(tmpln);
                    break;
            }
        }
        patterns.push_back(tmpblk);
    }
    file.close();
}

template<typename T>
void createXML(vector<T> patterns, const char* name){
    tinyxml2::XMLDocument ImageXML;
    tinyxml2::XMLNode *alto = ImageXML.NewElement("alto");
    ImageXML.InsertFirstChild(alto);
    
    tinyxml2::XMLNode *Layout = ImageXML.NewElement("Layout");
    alto->InsertFirstChild(Layout);
    
    tinyxml2::XMLElement *Page = ImageXML.NewElement("Page");
    Page->SetAttribute("HEIGHT", height);
    Page->SetAttribute("WIDTH", width);
    Layout->InsertFirstChild(Page);
    
    tinyxml2::XMLNode *PrintSpace = ImageXML.NewElement("PrintSpace");
    Page->InsertFirstChild(PrintSpace);
    
    tinyxml2::XMLNode *TextBlock = ImageXML.NewElement("TextBlock");
    PrintSpace->InsertFirstChild(TextBlock);
    
    tinyxml2::XMLNode *TextLine = ImageXML.NewElement("TextLine");
    TextBlock->InsertFirstChild(TextLine);
    
    //Child = Depth
    for(auto i:patterns){
        tinyxml2::XMLElement *String = ImageXML.NewElement("String");
        String->SetAttribute("HEIGHT", i.bottom - i.top);
        String->SetAttribute("WIDTH", i.right - i.left);
        String->SetAttribute("CONTENT", "NULL");
        String->SetAttribute("HPOS", i.left);
        String->SetAttribute("VPOS", i.top);
        TextLine->InsertEndChild(String);
    }
    ImageXML.SaveFile(name);
}

//1.改四分位数
//2.
int main(int argc, const char * argv[]) {
    
    //resize 二选一 下两行uncomment
//    Size size1(1000, 2000);
//    cv::resize(image_ori0, image_ori, size1);
    //the original size
    image_ori = image_ori0;
    
    height = image_ori.size().height;
    width = image_ori.cols;
    
//timer for the program
//    clock_t begin = clock();
    
//Binarize the image
    cvtColor(image_ori, imageBGR, CV_GRAY2BGR);
    threshold(image_ori, image, 128.0, 255.0, THRESH_BINARY);
    
//image baseline algorithm
//uncomment for the first time running*************************************
//    vector<rect> Rects = locate_rects();
//    vector<pattern> patterns = generate_patterns(Rects);
//    vector<pattern> lines;
//    vector<pattern> text;
//    vector<pattern> title;
//    vector<pattern> boxes = generate_boxes(patterns);
//*************************************************************************
    
//get and extend the lines in the patterns
//6.21 NOTES: some large patterns could be boxes which should be deleted
//    vector<pattern> lines = extract_line_boxes(boxes, 1);
//    vector<pattern> new_lines = reclass(patterns, lines);
//    vector<pattern> plines = get_line_from_patterns(patterns);
//    for(auto i:plines)
//        new_lines.push_back(i);

// uncomment for the first time running*****************************************
//      pattern_classfy(patterns, text, title);
//      saveInfo(text, "blocks0033text.txt");
//      saveInfo(title, "blocks0033title.txt");
//      saveInfo(patterns, "blocks0033patterns.txt");
//******************************************************************************
    
// revising params**************************************************************
//    read in the block information
    vector<pattern> text;
    readInfo(text, "blocks0033text.txt");
//    vector<pattern> title;
//    readInfo(title, "blocks0033title.txt");
//******************************************************************************
    
//    createXML(patterns, "blocks0033patterns.xml");
    
//    readInfo(patterns, "blocks0033patterns.txt");

      vector<block> textblocks = regionform(text);
//      vector<block> titleblocks = regionform(title);
//      saveInfo(textblocks, "textblocks0033.txt");
//      saveInfo(titleblocks, "titleblocks0033.txt");
    
//    cleanregion(blocks, 15, 0);
//    dealwithblocks(blocks, new_lines);
    
      Draw(textblocks, false, "textblocks00333");//false: do not print the blocks info
//      Draw(titleblocks, false, "titleblocks00333");
//    clock_t end = clock();
//    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
//    cout<<"The real running time in total is: "<<elapsed_secs<<"s"<<endl;

    return 0;
}
           
           
           
           
           
           
           
           
           
           
           
           
           
           
           
           
           
           
