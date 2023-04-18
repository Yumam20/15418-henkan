#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

std::vector<string> romajiMap; //stores romaji
std::vector<string> hiraganaMap; //stores hiragana 

void read_hira(){
    string line;
    vector<string> word;
    fstream file("hiraganaMap.csv", ios::in);
    if(file.is_open()){
        while(getline(file,line)){
            row.clear();
            stringstream str(line);
            getline(str,word,',');//int
            getline(str,word,',');//romaji
            romajiMap.push_back(word);
            getline(str,word,',');
        }
    }
}