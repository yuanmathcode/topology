#include<iostream>
#include<set>
#include<vector>
#include<map>
#include<memory>
#include<array>
#include<string>
#include<sstream>
#include<fstream>
#include<cstring>
#include<iomanip>
#include<algorithm>
#include "gurobi_c++.h"
#define M 1000000
using namespace std;
class node;
class edge;

class node{
private:
	int _id;
public:
	node()=default;
	node(int given_id)
	:_id(given_id){}
	const int &id() {return _id;}
	void define_id(int g_id){
		_id=g_id;}
};

class edge{
private:
	int _id;
	shared_ptr<node> _origin;
	shared_ptr<node> _end;
public:
	edge()=default;
	edge(int given_id,shared_ptr<node> given_origin,shared_ptr<node> given_end)
	:_id(given_id),_origin(given_origin),_end(given_end){}
	const int &id() {return _id;}
	shared_ptr<node> &origin() {return _origin;}
	shared_ptr<node> &end() {return _end;}
	void define_id(int g_id){
		_id=g_id;}
	void define_origin(shared_ptr<node> g_origin){
		_origin=g_origin;}
	void define_end(shared_ptr<node> g_end){
		_end=g_end;}
};

class variable{
public:
        int id;
        int type;//0=bool,1=int,2=double
        double upbound;
        double lobound;
        bool bool_value;
        int int_value;
        double double_value;
        set<int> constr_involved;
        map<int,double> constr_coeff;

};

int objcons;
int sum_constr;
map<int,int> constr_sense_map;
map<int,double> constr_rightside_map;
map<int,set<shared_ptr<variable>>> constr_variable_map;
vector<shared_ptr<variable>> vvar;
map<int,map<int,map<int,shared_ptr<variable>>>> color_map;//i:color,j:origin node,k:end node
//map<int,map<int,shared_ptr<variable>>> color_map;
vector<shared_ptr<node>> vecnode;
vector<shared_ptr<edge>> vecedge;
int adfloss;
int droploss;

void readfile(const char* filename){
	ifstream read(filename);
	string str_temp;
	int int_temp,int_temp1,int_temp2;
	double double_temp;
	while (read>> str_temp){
		//read node
		if(str_temp=="Node:"){
			while(read>>str_temp){
				if(str_temp=="end")
					break;
			int_temp=stoi(str_temp);
			shared_ptr<node> node_temp=make_shared<node>(int_temp);
			vecnode.push_back(node_temp);
			}
		}
		//Read edge
		if(str_temp=="Edge:"){
			int index_temp=0;
			while(getline(read,str_temp)){
				if(str_temp=="end")
					break;
				if(str_temp.empty())
					continue;
			index_temp++;
			istringstream edge_record(str_temp);
			edge_record>>int_temp1>>int_temp2;
			shared_ptr<node> node_temp1, node_temp2;
			for(auto &i:vecnode){
				if(i->id()==int_temp1)
					node_temp1=i;
			}
			for(auto &j:vecnode){
				if(j->id()==int_temp2)
					node_temp2=j;
			}
			shared_ptr<edge> edge_temp=make_shared<edge>(index_temp,node_temp1,node_temp2);
			vecedge.push_back(edge_temp);
			}
		}
                if(str_temp=="Adf:"){
                        while(read>>str_temp){
                                if(str_temp=="end")
                                        break;
                        int_temp=stoi(str_temp);
                        adfloss=int_temp;
			}
                }
                if(str_temp=="Drop:"){
                        while(read>>str_temp){
                                if(str_temp=="end")
                                        break;
                        int_temp=stoi(str_temp);
                        droploss=int_temp;
			}
                }
		
	}
}

//define color_map[i][j][h] as binary variable:i means color,j means node;
void set_color_map(){
	for(int l=1;l<=vecnode.size();l++){
		for(int h=1;h<=vecnode.size();h++){
			for(int i=0;i<=vecnode.size();i++){
				color_map[i][l][h]=make_shared<variable>();
				color_map[i][l][h]->id=vvar.size();
				color_map[i][l][h]->type=0;
				color_map[i][l][h]->upbound=1;
				color_map[i][l][h]->lobound=0;
				vvar.push_back(color_map[i][l][h]);
			}
		}
	}
}

//each edge can only has one color;
void set_constraint_group1(){
	for(auto &j:vecedge){
                sum_constr++;
                constr_sense_map[sum_constr]=0;
                constr_rightside_map[sum_constr]=1;
		constr_variable_map[sum_constr].insert(color_map[0][j->origin()->id()][j->end()->id()]);
                color_map[0][j->origin()->id()][j->end()->id()]->constr_involved.insert(sum_constr);
                color_map[0][j->origin()->id()][j->end()->id()]->constr_coeff[sum_constr]=1;
		for(int i=1;i<=vecnode.size();i++){
                        constr_variable_map[sum_constr].insert(color_map[i][j->origin()->id()][j->end()->id()]);
                        color_map[i][j->origin()->id()][j->end()->id()]->constr_involved.insert(sum_constr);
                        color_map[i][j->origin()->id()][j->end()->id()]->constr_coeff[sum_constr]=1;
                }
        }
}
//each row and column must has different color
void set_constraint_group2(){
	for(int k=0;k<=vecnode.size();k++){
		for(auto &i:vecnode){
			sum_constr++;
			constr_sense_map[sum_constr]=-1;
			constr_rightside_map[sum_constr]=1;
			for(auto &j:vecnode){	
				constr_variable_map[sum_constr].insert(color_map[k][i->id()][j->id()]);
				color_map[k][i->id()][j->id()]->constr_involved.insert(sum_constr);
				color_map[k][i->id()][j->id()]->constr_coeff[sum_constr]=1;
			}
		}
	}

	for(int k=0;k<=vecnode.size();k++){
		for(auto &j:vecnode){
			sum_constr++;
			constr_sense_map[sum_constr]=-1;
			constr_rightside_map[sum_constr]=1;
			for(auto &i:vecnode){	
				constr_variable_map[sum_constr].insert(color_map[k][i->id()][j->id()]);
				color_map[k][i->id()][j->id()]->constr_involved.insert(sum_constr);
				color_map[k][i->id()][j->id()]->constr_coeff[sum_constr]=1;
			}
		}
	}
}
map<int,map<int,map<int,map<int,shared_ptr<variable>>>>> share;//share_ihjl;j=i2,i=i1,l=j2,h=j1
void set_sharedvalue(){
        for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){         
					share[i][h][j][l]=make_shared<variable>();
                   			share[i][h][j][l]->id=vvar.size();
                        		share[i][h][j][l]->type=0;
                        		share[i][h][j][l]->upbound=1;
                        		share[i][h][j][l]->lobound=0;
                        		vvar.push_back(share[i][h][j][l]);
				}
			}
		}
        }
}
///constraint 78910
void set_constraint_group3(){
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
                        		sum_constr++;
                        		constr_sense_map[sum_constr]=1;
                        		constr_rightside_map[sum_constr]=0;
                        		constr_variable_map[sum_constr].insert(color_map[0][i][l]);
                        		color_map[0][j][h]->constr_involved.insert(sum_constr);
                        		color_map[0][j][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-1;
                		}
			}
		}
        }
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
                        		sum_constr++;
                        		constr_sense_map[sum_constr]=1;
                        		constr_rightside_map[sum_constr]=0;
                        		constr_variable_map[sum_constr].insert(color_map[0][j][h]);
                        		color_map[0][j][h]->constr_involved.insert(sum_constr);
                        		color_map[0][j][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-1;
		                }       
			}
		}
        }
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=M;     
       		        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
       	        			share[i][h][j][l]->constr_involved.insert(sum_constr);
       		        		share[i][h][j][l]->constr_coeff[sum_constr]=M;
					for(int k=1;k<=vecnode.size();k++){
						constr_variable_map[sum_constr].insert(color_map[k][i][h]);
						color_map[k][i][h]->constr_involved.insert(sum_constr);
						color_map[k][i][h]->constr_coeff[sum_constr]=k;
                                		constr_variable_map[sum_constr].insert(color_map[k][j][l]);
                                		color_map[k][j][l]->constr_involved.insert(sum_constr);
                                		color_map[k][j][l]->constr_coeff[sum_constr]=-k;
					}
				}
			}
		}
	}	
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=M;     
       		        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
       	        			share[i][h][j][l]->constr_involved.insert(sum_constr);
       		        		share[i][h][j][l]->constr_coeff[sum_constr]=M;
					for(int k=1;k<=vecnode.size();k++){
						constr_variable_map[sum_constr].insert(color_map[k][j][l]);
						color_map[k][j][l]->constr_involved.insert(sum_constr);
						color_map[k][j][l]->constr_coeff[sum_constr]=k;
                                		constr_variable_map[sum_constr].insert(color_map[k][i][h]);
                                		color_map[k][i][h]->constr_involved.insert(sum_constr);
                                		color_map[k][i][h]->constr_coeff[sum_constr]=-k;
					}
				}
			}
		}
	}
}
//define binary variable for ADF
map<int,map<int,shared_ptr<variable>>> adf;
void set_adf(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			adf[i][j]=make_shared<variable>();
			adf[i][j]->id=vvar.size();
			adf[i][j]->type=0;
			adf[i][j]->upbound=1;
			adf[i][j]->lobound=0;
			vvar.push_back(adf[i][j]);
		}
	}
}

//constraint 11; i,j means ij h,l means int a,b
void set_constraint_group4(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=1;
			constr_rightside_map[sum_constr]=0;
			constr_variable_map[sum_constr].insert(adf[i][j]);
			adf[i][j]->constr_involved.insert(sum_constr);
			adf[i][j]->constr_coeff[sum_constr]=1;
			for(int k=1;k<=vecnode.size();k++){
                                constr_variable_map[sum_constr].insert(color_map[k][i][j]);
                                color_map[k][i][j]->constr_involved.insert(sum_constr);
                                color_map[k][i][j]->constr_coeff[sum_constr]=-1;
			}
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){		
                        		constr_variable_map[sum_constr].insert(share[i][j][h][l]);
                        		share[i][j][h][l]->constr_involved.insert(sum_constr);
                        		share[i][j][h][l]->constr_coeff[sum_constr]=1;
				}
			}
		}
	}
}
//constraint 12 13;i:i1 h:j1 j:i2 l:j2
void set_constraint_group5(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
					sum_constr++;
					constr_sense_map[sum_constr]=-1;
					constr_rightside_map[sum_constr]=2;
					constr_variable_map[sum_constr].insert(adf[i][h]);
					adf[i][h]->constr_involved.insert(sum_constr);
					adf[i][h]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(adf[j][l]);
					adf[j][l]->constr_involved.insert(sum_constr);
					adf[j][l]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(share[i][h][j][l]);
					share[i][h][j][l]->constr_involved.insert(sum_constr);
					share[i][h][j][l]->constr_coeff[sum_constr]=-1;
				}
			}
		}
	}
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){			
					sum_constr++;
					constr_sense_map[sum_constr]=1;
					constr_rightside_map[sum_constr]=0;
					constr_variable_map[sum_constr].insert(adf[i][h]);
					adf[i][h]->constr_involved.insert(sum_constr);
					adf[i][h]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(adf[j][l]);
					adf[j][l]->constr_involved.insert(sum_constr);
					adf[j][l]->constr_coeff[sum_constr]=1;
					constr_variable_map[sum_constr].insert(share[i][h][j][l]);
					share[i][h][j][l]->constr_involved.insert(sum_constr);
					share[i][h][j][l]->constr_coeff[sum_constr]=-1;
				}
			}
		}
	}
}
//define insertion loss 
map<int,map<int,shared_ptr<variable>>> Il;//insertionloss
void set_insertionloss(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
        		Il[i][j]=make_shared<variable>();
        		Il[i][j]->id=vvar.size();
        		Il[i][j]->type=1;
        		Il[i][j]->upbound=10000;
        		Il[i][j]->lobound=0;
        		vvar.push_back(Il[i][j]);
		}
	}
}

//define adf number in the path
map<int,map<int,shared_ptr<variable>>> numadf;//number of adf
void set_numofadf(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
        		numadf[i][j]=make_shared<variable>();
        		numadf[i][j]->id=vvar.size();
        		numadf[i][j]->type=1;
        		numadf[i][j]->upbound=10000;
        		numadf[i][j]->lobound=0;
        		vvar.push_back(numadf[i][j]);
		}
	}
}

//set constraint 14
void set_constraint_group6(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=0;
			constr_rightside_map[sum_constr]=droploss-adfloss;
			constr_variable_map[sum_constr].insert(Il[i][j]);
			Il[i][j]->constr_involved.insert(sum_constr);
			Il[i][j]->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(numadf[i][j]);
			numadf[i][j]->constr_involved.insert(sum_constr);
			numadf[i][j]->constr_coeff[sum_constr]=-1*adfloss;
			constr_variable_map[sum_constr].insert(color_map[0][i][j]);
			color_map[0][i][j]->constr_involved.insert(sum_constr);
			color_map[0][i][j]->constr_coeff[sum_constr]=droploss-adfloss;
		}
	}
}
//default path constraint 15
void set_constraint_group7(){
	for(int i=1;i<=vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){	
        		sum_constr++;
        		constr_sense_map[sum_constr]=1;
        		constr_rightside_map[sum_constr]=-M;
       			constr_variable_map[sum_constr].insert(numadf[i][j]);
        		numadf[i][j]->constr_involved.insert(sum_constr);
			numadf[i][j]->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(color_map[0][i][j]);
			color_map[0][i][j]->constr_involved.insert(sum_constr);
			color_map[0][i][j]->constr_coeff[sum_constr]=-M;
			for(int k=1;k<=vecnode.size();k++){
       				constr_variable_map[sum_constr].insert(adf[i][k]);
        			adf[i][k]->constr_involved.insert(sum_constr);
				adf[i][k]->constr_coeff[sum_constr]=-1;
				constr_variable_map[sum_constr].insert(adf[k][j]);
				adf[k][j]->constr_involved.insert(sum_constr);
				adf[k][j]->constr_coeff[sum_constr]=-1;
			}
		}			
        }
}
//constraint 16 
void set_constraint_group8(){
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){    
                        sum_constr++;
                        constr_sense_map[sum_constr]=1;
                        constr_rightside_map[sum_constr]=-M;
                        constr_variable_map[sum_constr].insert(numadf[i][j]);
                        numadf[i][j]->constr_involved.insert(sum_constr);
                        numadf[i][j]->constr_coeff[sum_constr]=1;
                        constr_variable_map[sum_constr].insert(adf[i][j]);
                        adf[i][j]->constr_involved.insert(sum_constr);
                        adf[i][j]->constr_coeff[sum_constr]=-M;
                        for(int k=1;k<=j;k++){
                                constr_variable_map[sum_constr].insert(adf[i][k]);
                                adf[i][k]->constr_involved.insert(sum_constr);
                                adf[i][k]->constr_coeff[sum_constr]=-1;
			}
			for(int k=1;k<i;k++){
                                constr_variable_map[sum_constr].insert(adf[k][j]);
                                adf[k][j]->constr_involved.insert(sum_constr);
                                adf[k][j]->constr_coeff[sum_constr]=-1;
                        }   
                }    
        }   
}
//question i+1 j+1 loop problem
void set_constraint_group9(){
        for(int i=1;i<=vecnode.size();i++){
                for(int j=1;j<=vecnode.size();j++){
			for(int h=1;h<=vecnode.size();h++){
				for(int l=1;l<=vecnode.size();l++){
                        		sum_constr++;
                        		constr_sense_map[sum_constr]=1;
                        		constr_rightside_map[sum_constr]=-M;
                    		    	constr_variable_map[sum_constr].insert(numadf[i][h]);
                        		numadf[i][h]->constr_involved.insert(sum_constr);
                        		numadf[i][h]->constr_coeff[sum_constr]=1;
                        		constr_variable_map[sum_constr].insert(adf[i][h]);
                        		adf[i][h]->constr_involved.insert(sum_constr);
                        		adf[i][h]->constr_coeff[sum_constr]=M;
                        		constr_variable_map[sum_constr].insert(share[i][h][j][l]);
                        		share[i][h][j][l]->constr_involved.insert(sum_constr);
                        		share[i][h][j][l]->constr_coeff[sum_constr]=-M;
                        		for(int k=1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[i][k]);
                                		adf[i][k]->constr_involved.insert(sum_constr);
                                		adf[i][k]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=h;k<=vecnode.size();k++){
                               			constr_variable_map[sum_constr].insert(adf[k][l]);
                                		adf[k][l]->constr_involved.insert(sum_constr);
                                		adf[k][l]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=l+1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[k][j]);
                                		adf[k][j]->constr_involved.insert(sum_constr);
                                		adf[k][j]->constr_coeff[sum_constr]=-1;
                        		}
                        		for(int k=1;k<=vecnode.size();k++){
                                		constr_variable_map[sum_constr].insert(adf[k][h]);
                                		adf[k][h]->constr_involved.insert(sum_constr);
                                		adf[k][h]->constr_coeff[sum_constr]=-1;
                        		}
				}
			}
                }
        }
}
//define total number of adfs
shared_ptr<variable> totaladf;
shared_ptr<variable> totallambda;
shared_ptr<variable> totalloss;
void set_totaladf(){
        totaladf=make_shared<variable>();
        totaladf->id=vvar.size();
        totaladf->type=1;
        totaladf->upbound=10000;
        totaladf->lobound=0;
        vvar.push_back(totaladf);
}
void set_totalwavelength(){
        totallambda=make_shared<variable>();
        totallambda->id=vvar.size();
        totallambda->type=1;
        totallambda->upbound=10000;
        totallambda->lobound=0;
        vvar.push_back(totallambda);
}
void set_totalloss(){
        totalloss=make_shared<variable>();
        totalloss->id=vvar.size();
        totalloss->type=1;
        totalloss->upbound=10000;
        totalloss->lobound=0;
        vvar.push_back(totalloss);
}
//constraint 21
void set_constraint_group10(){
	sum_constr++;
        constr_sense_map[sum_constr]=0;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(totaladf);
        totaladf->constr_involved.insert(sum_constr);
        totaladf->constr_coeff[sum_constr]=1;
	for(auto &i:vecnode){
		for(auto &j:vecnode){
        		constr_variable_map[sum_constr].insert(adf[i->id()][j->id()]);
                        adf[i->id()][j->id()]->constr_involved.insert(sum_constr);
                        adf[i->id()][j->id()]->constr_coeff[sum_constr]=-1;
		}
	}
}
//constraint 22
void set_constraint_group11(){
	sum_constr++;
        constr_sense_map[sum_constr]=1;
        constr_rightside_map[sum_constr]=0;
        constr_variable_map[sum_constr].insert(totallambda);
        totallambda->constr_involved.insert(sum_constr);
        totallambda->constr_coeff[sum_constr]=1;
	for(int k=1;k<=vecnode.size();k++){
		for(auto &i:vecnode){
			for(auto &j:vecnode){
        		constr_variable_map[sum_constr].insert(color_map[k][i->id()][j->id()]);
                        color_map[k][i->id()][j->id()]->constr_involved.insert(sum_constr);
                        color_map[k][i->id()][j->id()]->constr_coeff[sum_constr]=-k;
			}
		}
	}
}
void set_constraint_group12(){
	for(int i=1;i<vecnode.size();i++){
		for(int j=1;j<=vecnode.size();j++){
			sum_constr++;
			constr_sense_map[sum_constr]=1;
			constr_rightside_map[sum_constr]=0;
			constr_variable_map[sum_constr].insert(totalloss);
			totalloss->constr_involved.insert(sum_constr);
			totalloss->constr_coeff[sum_constr]=1;
			constr_variable_map[sum_constr].insert(Il[i][j]);
			Il[i][j]->constr_involved.insert(sum_constr);
			Il[i][j]->constr_coeff[sum_constr]=-1;
			}
	}
	sum_constr++;
        constr_variable_map[sum_constr].insert(totallambda);
        totallambda->constr_involved.insert(sum_constr);
        totallambda->constr_coeff[sum_constr]=10;
        constr_variable_map[sum_constr].insert(totaladf);
        totaladf->constr_involved.insert(sum_constr);
        totaladf->constr_coeff[sum_constr]=10;
        constr_variable_map[sum_constr].insert(totalloss);
        totalloss->constr_involved.insert(sum_constr);
        totalloss->constr_coeff[sum_constr]=100;
}
void funcGurobi(double time, double absgap, int idisplay)
{
        GRBEnv env = GRBEnv();
        GRBModel model = GRBModel(env);
        model.getEnv().set(GRB_DoubleParam_TimeLimit, time); // case 900
//      model.getEnv().set(GRB_DoubleParam_MIPGapAbs, 0); // case 0.02
//      model.getEnv().set(GRB_DoubleParam_Heuristics, 1); // case none
//      model.getEnv().set(GRB_DoubleParam_ImproveStartGap, 0.02); // case 0.02
        model.getEnv().set(GRB_IntParam_OutputFlag, idisplay);
        map<shared_ptr<variable>,string> mapvs;
        for(int i=0;i<vvar.size();i++)
        {
                ostringstream convi;
                convi<<i;
                mapvs[vvar[i]]="x"+convi.str();
        }
        GRBVar *x=new GRBVar [vvar.size()+1];
        for(int i=0;i<vvar.size();i++)
        {
                if(vvar[i]->type==0)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_BINARY,mapvs[vvar[i]]);
                else if(vvar[i]->type==1)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_INTEGER,mapvs[vvar[i]]);
                else if(vvar[i]->type==2)
                x[i]=model.addVar(vvar[i]->lobound,vvar[i]->upbound,0.0,GRB_CONTINUOUS,mapvs[vvar[i]]);
        }
        model.update();
        for(int i=1;i<=sum_constr;i++)
        if(constr_variable_map[i].size()!=0) // cons with 0 var is eliminated
        {
                ostringstream convi;
                convi<<i;
                GRBLinExpr expr;

                if(i!=objcons)
                {
                        for(auto setit:constr_variable_map[i])
                        {
                                expr+=x[setit->id]*(setit->constr_coeff[i]);
                                //expr+=x[(*setit)->index]*((*setit)->constr_coeff[i]);
                        }
                        string name='c'+convi.str();
                        if(constr_sense_map[i]==1)
                                model.addConstr(expr,GRB_GREATER_EQUAL,constr_rightside_map[i],name);
                        else if(constr_sense_map[i]==-1)
                                model.addConstr(expr,GRB_LESS_EQUAL,constr_rightside_map[i],name);
                        else if(constr_sense_map[i]==0)
                                model.addConstr(expr,GRB_EQUAL,constr_rightside_map[i],name);
                }
                else
                {
                        for(auto setit:constr_variable_map[i])
			{
//                        for(set<variable*>::iterator setit=constr_variable_map[i].begin();setit!=constr_variable_map[i].end();setit++)
                                expr+=x[setit->id]*(setit->constr_coeff[i]);
                //      model.setObjective(expr,GRB_MAXIMIZE);
                                model.setObjective(expr,GRB_MINIMIZE);
                	}
        	}
	}
//      mycallback cb(time, absgap);
//      model.setCallback(&cb);
        model.optimize();

        while(model.get(GRB_IntAttr_SolCount)==0)
        {
                time+=5;
                model.getEnv().set(GRB_DoubleParam_TimeLimit, time); // case 900
                model.optimize();
        }
        for(int i=0;i<vvar.size();i++)
        {
                if(vvar[i]->type==0||vvar[i]->type==1)
                {
                        if(x[i].get(GRB_DoubleAttr_X)-(int)x[i].get(GRB_DoubleAttr_X)<0.5)
                                vvar[i]->int_value=(int)x[i].get(GRB_DoubleAttr_X);
                        else
                                vvar[i]->int_value=(int)x[i].get(GRB_DoubleAttr_X)+1;
                }
                else if(vvar[i]->type==2)
                        vvar[i]->double_value=x[i].get(GRB_DoubleAttr_X);
                else
                        cout<<"new type"<<endl;
        }
}

int main(int argc, char * argv[]){
	readfile("input.txt");
	set_color_map();
	set_sharedvalue();
	set_adf();
	set_numofadf();
	set_insertionloss();
	set_totalwavelength();
	set_totaladf();
	set_totalloss();
	set_constraint_group1();
	set_constraint_group2();
	set_constraint_group3();
	set_constraint_group4();
	set_constraint_group5();
	set_constraint_group6();
	set_constraint_group7();
	set_constraint_group8();
	set_constraint_group9();
	set_constraint_group10();	
	set_constraint_group11();
	set_constraint_group12();
	objcons=sum_constr;
	funcGurobi(30,0,1);
//	cout<<adfloss<<" "<<droploss<<" "<<endl;
	cout<<totaladf->int_value<<" "<<totallambda->int_value<<"  "<<totalloss->int_value<<" "<<endl;
	cout<<totaladf->int_value+totallambda->int_value+totalloss->int_value<<"  "<<endl;
/*	for(int i=0;i<25;i++)
	{
		cout<<y[i]->int_value<<" "<<endl;
		cout<<"{";
		for(auto &j:vecedge)
		{		
			cout<<"("<<j->origin()->id()<<","<<j->end()->id()<<")"<<color_map[i][j->origin()->id()][j->end()->id()]->int_value<<", ";

		}
	cout<<"}"<<endl;
	}*/
}	
