#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include "timing.h"


using namespace std;

std::map<std::string, std::string> romajiHiraganaMap;
void read_hira(){
    string line;
    string word;
    string romaji;
    string hiragana;
    ifstream file;
    file.open("hiraganaMap.csv");
    //cout << "hello?" << endl;
     if (!file.is_open())
    {
        cout << "path is wrong for map" << endl;
        return;
    }
    while(getline(file,line)){
        istringstream str(line);
        //cout << line << endl;
        getline(str,word,',');//int
        getline(str,romaji,',');//romaji
        //cout << "word " << romaji << endl;
        getline(str,word,',');
        getline(str,hiragana,',');//romaji
        //cout << "word " << hiragana << endl;
        romajiHiraganaMap[romaji] = hiragana;
    }

    // Iterate through the map and print the elements
    std::map<std::string, std::string>::iterator it;
    for (it = romajiHiraganaMap.begin(); it != romajiHiraganaMap.end(); ++it);
    {
        //cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
    }
    cout << endl;
}
bool inDict(string queryString){
    return romajiHiraganaMap.find(queryString) != romajiHiraganaMap.end();
}

string naiveHenkan(string inputString){
    //cout << inputString << std::endl;
    if(inputString.size() == 0){
        return "";
    }
    if(inputString[0] == ' '){
        return " " + naiveHenkan(inputString.substr(1,inputString.size()-1));
    }
    //need to consider cases with nobashi-bou and other typing standards resulting in smaller characters
    if(inputString[0] == inputString[1]){
        //first two letters are the same -> insert little tsu
        return "っ" + naiveHenkan(inputString.substr(1,inputString.size()-1));
    }
    //cout << "substring " << inputString << " " << endl;
    //
    for(size_t i = inputString.size() ; i >0 ; i--){
        //cout << "trying " << inputString.substr(0,i) << std::endl;


        if(inDict(inputString.substr(0,i))) {
            // mapping  found
            //cout<< "found " << romajiHiraganaMap[inputString.substr(0,i)] << " " << std::endl;
            string recursiveCall = naiveHenkan(inputString.substr(i,inputString.size()-i));
            //cout << "searching" << inputString.substr(i,inputString.size()-i) << " " << std::endl;
            if(true){
                return romajiHiraganaMap[inputString.substr(0,i)] + recursiveCall;
            }

        }
    }
    return inputString;
}

string testHenkan(string inputString){
    //int middle = inputString.size() / 2;
    int frontBound, endBound;
    frontBound = 0; endBound = inputString.size();
    string returnMe = "";

    while(frontBound < endBound){
        for(int i = 3; i > 0; i++){ //3 is maxsize romaji in dict

            if(inDict(inputString.substr(frontBound,i))){
                returnMe.insert(0,romajiHiraganaMap[inputString.substr(frontBound,i)]);
                frontBound += i;
            }

            if(inDict(inputString.substr(endBound-i,i))){
                returnMe += romajiHiraganaMap[inputString.substr(frontBound,i)];
                endBound -= i;
            }
        } //might want to break up for loops to ensure that 1 is not stall waiting for other if already found
    }
    return returnMe;

}
/*
string parallelHenkan(string inputString){
    for(size_t chunk = 3; chunk >= 0; chunk--){
        #pragma omp parallel
        {
            for(int i = 0 ; i < (inputString.size() / chunk) ; i += chunk){
                if(romajiHiraganaMap.find(inputString.substr(i,chunk)) != romajiHiraganaMap.end()) {
                
                }
            } 
        }
    }
    return "\n";
    //pragma omp omp
}*/


int main() {
    string inputString;
    string outputString;
    cout << "Please Enter the Input" << std::endl;
    getline(cin, inputString);
    boost::algorithm::to_lower(inputString);
    read_hira();
    //insert timing code
    Timer totalSimulationTimer;
    string naiveOutput = naiveHenkan(inputString);
    double totalSimulationTime = totalSimulationTimer.elapsed();
    printf("total simulation time: %.6fs\n", totalSimulationTime);
    //insert timing code
    //string parallelOutput = parallelHenkan(inputString);

    string testOutput = testHenkan(inputString);
    //insert timing code
    cout << "naiveHenkan: " << naiveOutput << std::endl;
    //cout << "parallelHenkan: " << parallelOutput << std::endl;
    cout << "testHenkan: " << testOutput << std::endl;
    return 0;
}

// eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen

