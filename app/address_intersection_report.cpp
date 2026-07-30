#include "bitcoin/address/format.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
using Map=std::map<std::string,std::vector<size_t>>;
static Map read(const char* path,size_t& valid,size_t& invalid){std::ifstream in(path);if(!in)throw std::runtime_error("could not open input");Map r;std::string l;size_t n=0;while(std::getline(in,l)){++n;auto x=crypto_research::bitcoin::address::inspect_public_address(l);if(x.normalized.empty())continue;if(x.valid){++valid;r[x.normalized].push_back(n);}else ++invalid;}return r;}
int main(int argc,char* argv[]){if(argc!=9||std::string_view(argv[1])!="--left"||std::string_view(argv[3])!="--right"||std::string_view(argv[5])!="--json-report"||std::string_view(argv[7])!="--csv"){std::cerr<<"Usage: address_intersection_report --left <file> --right <file> --json-report <file> --csv <file>\n";return 2;}try{size_t lv=0,li=0,rv=0,ri=0;auto l=read(argv[2],lv,li),r=read(argv[4],rv,ri);nlohmann::json rows=nlohmann::json::array();std::ofstream csv(argv[8]);csv<<"address,type,left_lines,right_lines\n";for(auto&[a,ll]:l)if(auto it=r.find(a);it!=r.end()){auto type=crypto_research::bitcoin::address::to_string(crypto_research::bitcoin::address::inspect_public_address(a).type);rows.push_back({{"address",a},{"type",type},{"left_lines",ll},{"right_lines",it->second}});csv<<a<<","<<type<<",\""<<ll.front()<<"\",\""<<it->second.front()<<"\"\n";}std::ofstream out(argv[6]);out<<nlohmann::json{{"tool","address_intersection_report"},{"summary",{{"valid_left",lv},{"valid_right",rv},{"invalid_left",li},{"invalid_right",ri},{"intersection_count",rows.size()}}},{"matches",rows}}.dump(2)<<'\n';}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
