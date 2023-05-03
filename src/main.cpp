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
enum dictStatus {FOUND_DICT, FOUND_EXCEPTION, NOT_FOUND};
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

string romajiExceptions(string queryString){
    if (queryString == " "){
        return " ";
    }
    if(queryString.size() == 2 && queryString[0] == queryString[1]){
        return "っ";
    }
    if(queryString == "-"){
        return "ー";
    }
    return ""; //return "" if its not an exception

}

dictStatus inDict(string queryString){
    if((romajiHiraganaMap.find(queryString) != romajiHiraganaMap.end())){
        return FOUND_DICT;
    }
    else if (romajiExceptions(queryString) != ""){
        return FOUND_EXCEPTION;
    }
    return NOT_FOUND;
}

string retrieveDict(string queryString){
    cout << "found " << queryString << "\n" << std::endl;
    dictStatus qs = inDict(queryString);
    if(qs == FOUND_DICT){
        return romajiHiraganaMap[queryString];
    }
    else if(qs == FOUND_EXCEPTION){
        return romajiExceptions(queryString);
    }
    //should not hit this case
    return "bruh";


}





string naiveHenkan(string inputString){
    //cout << inputString << std::endl;
    if(inputString.size() == 0){
        return "";
    }
    //cout << "substring " << inputString << " " << endl;
    //
    for(size_t i = inputString.size() ; i >0 ; i--){
        //cout << "trying " << inputString.substr(0,i) << std::endl;


        if(inDict(inputString.substr(0,i)) != NOT_FOUND) {
            // mapping  found
            //cout<< "found " << romajiHiraganaMap[inputString.substr(0,i)] << " " << std::endl;
            string recursiveCall = naiveHenkan(inputString.substr(i,inputString.size()-i));
            //cout << "searching" << inputString.substr(i,inputString.size()-i) << " " << std::endl;
            return retrieveDict(inputString.substr(0,i)) + recursiveCall;

        }
    }
    return inputString;
}

string edgeHenkan(string inputString){
    return "";
    //int middle = inputString.size() / 2;
    int frontBound, endBound;
    frontBound = 0; endBound = inputString.size();
    string returnMe = "";
    int frontInsertPosition = 0; 
    int backInsertPosition = 0;

    while(frontBound < endBound){
        for(int i = 3; i > 0; i--){ //3 is maxsize romaji in dict
            //cout << "trying frontBound" << inputString.substr(frontBound,i) << std::endl;
            if(inDict(inputString.substr(frontBound,i))){
                //cout << "found frontBound" << inputString.substr(frontBound,i) << std::endl;
                returnMe.insert(frontInsertPosition,romajiHiraganaMap[inputString.substr(frontBound,i)]);
                frontInsertPosition += romajiHiraganaMap[inputString.substr(frontBound,i)].size();
                backInsertPosition = max(backInsertPosition, frontInsertPosition);
                frontBound += i;
            }
            //cout << "trying endBound " << inputString.substr(endBound-i,i) << std::endl;
            if(inDict(inputString.substr(endBound-i,i))){
                //cout << "found endBound " << inputString.substr(endBound-i,i) << std::endl;
                returnMe += romajiHiraganaMap[inputString.substr(frontBound,i)];
                backInsertPosition -= romajiHiraganaMap[inputString.substr(frontBound,i)].size();
                backInsertPosition = min(backInsertPosition, frontInsertPosition);
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
void dut(string inputString){
    Timer totalSimulationTimer;
    string naiveOutput = naiveHenkan(inputString);
    double naiveTime = totalSimulationTimer.elapsed();
    string edgeOutput = edgeHenkan(inputString);
    double totalSimulationTime = totalSimulationTimer.elapsed();
    printf("total simulation time: %.6fs\n", totalSimulationTime);
    printf("naiveHenkan simulation time: %.6fs\n", naiveTime);
    printf("edgeHenkan simulation time: %.6fs\n", totalSimulationTime-naiveTime);
    printf("Speedup: %f\n", (naiveTime/(totalSimulationTime-naiveTime)));
    //insert timing code
    cout << "naiveHenkan: " << naiveOutput << std::endl;
    //cout << "parallelHenkan: " << parallelOutput << std::endl;
    cout << "edgeHenkan: " << edgeOutput << std::endl;
}

int main() {
    string inputString;
    cout << "Please Enter the Input" << std::endl;
    getline(cin, inputString);
    boost::algorithm::to_lower(inputString);
    read_hira();

    dut(inputString);

    return 0;
}

// eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen

