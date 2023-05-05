#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include "timing.h"
#include <omp.h>
#include <algorithm>
#include <string>


using namespace std;

int GRANULARITY_BOUND = 4;
int LOAD_BALANCE = 200;

std::map<std::string, std::string> romajiHiraganaMap;
enum dictStatus {FOUND_DICT, FOUND_EXCEPTION, NOT_FOUND};
string edgeHenkan(string inputString);
string edgeHenkanParallel(string inputString);

// helper function to read input file and return as string
string read_input_file(string inputSrc) {
    ifstream file;
    file.open(inputSrc); //open the input file
    if (!file.is_open())
    {
        cerr << "path is wrong for input file" << endl;
        return "";
    }
    stringstream strStream;
    strStream << file.rdbuf(); //read the file
    string file_string = strStream.str();

   return file_string;
}

// helper function to read in hiragana mappings
// from spreadsheet
void read_hira() {
    string line;
    string word;
    string romaji;
    string hiragana;
    ifstream file;

    file.open("hiraganaMap.csv");

    if (!file.is_open())
    {
        cout << "path is wrong for map" << endl;
        return;
    }

    // read in lines
    while(getline(file,line)){
        istringstream str(line);

        getline(str,word,',');//int
        getline(str,romaji,',');//romaji
        getline(str,hiragana,',');//romaji
        romajiHiraganaMap[romaji] = hiragana;
    }
}

string romajiExceptions(string queryString){
    if (queryString == " "){
        return " ";
    }
    else if(queryString.size() == 4 && queryString[0] == queryString[1] && romajiHiraganaMap.find(queryString.substr(1,3)) != romajiHiraganaMap.end()){
        return "っ" + romajiHiraganaMap[queryString.substr(1,3)];
    }
    else if(queryString.size() == 3 && queryString[0] == queryString[1] && romajiHiraganaMap.find(queryString.substr(1,2)) != romajiHiraganaMap.end()){
        return "っ" + romajiHiraganaMap[queryString.substr(1,2)];
    }
    else if(queryString == "-"){
        return "ー";
    }
    else if(queryString.size() == 1 && ((queryString[0] < 'a' || queryString[0] > 'z') && (queryString[0] < 'A' || queryString[0] > 'Z'))){
        return queryString; //not in alphabet (i.e. numeric, special characters like $ @ %)
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

string sequentialEdgeHenkan(string inputString) {
    vector<string> henkan_list;
    string final_henkan;
    istringstream ss(inputString);
    string word; // for storing each word
    // Traverse through all words
    // while loop till we get
    // strings to store in string word
    string temp = "";
    while (ss >> word)
    {
        temp += word + " ";
        if (temp.size() >= LOAD_BALANCE) {
            henkan_list.emplace_back(temp);
            temp = "";
        }
    }
    if (temp != "") {
        henkan_list.emplace_back(temp);
    }
    vector<string> henkan_out = vector<string>(henkan_list.size());
    for (int i = 0; i < henkan_list.size(); i++) {
        int thread_num = omp_get_thread_num();
        // std::cout << thread_num << henkan_list[i] << std::endl;
        henkan_out[i] = edgeHenkan(henkan_list[i]);
    }

    final_henkan = boost::algorithm::join(henkan_out, "");

   
    return final_henkan;
}
string paralleHenkanEdgeParallelism(string inputString) {
    string final_henkan;
    std::cout << "hello " << inputString << endl;
    final_henkan = edgeHenkanParallel(inputString);
   
    return final_henkan;
}

string parallelEdgeHenkanNoParallel(string inputString) {
    vector<string> henkan_list;
    string final_henkan;
    istringstream ss(inputString);
    string word; // for storing each word
    // Traverse through all words
    // while loop till we get
    // strings to store in string word
    string temp = "";
    while (ss >> word)
    {
        temp += word + "";
        if (temp.size() >= LOAD_BALANCE) {
            henkan_list.emplace_back(temp);
            temp = "";
        }
    }
    if (temp != "") {
        henkan_list.emplace_back(temp);
    }
    vector<string> henkan_out = vector<string>(henkan_list.size());
    if (henkan_list.size() >= 2) {
         #pragma omp parallel
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < henkan_list.size(); i++) {
                int thread_num = omp_get_thread_num();
                // std::cout << thread_num << henkan_list[i] << std::endl;
                henkan_out[i] = edgeHenkan(henkan_list[i]);
            }
        }
        final_henkan = boost::algorithm::join(henkan_out, "");
    } else {
        final_henkan =  edgeHenkan(henkan_list[0]);
    }

   
    return final_henkan;
}

string parallelEdgeHenkan(string inputString) {
    vector<string> henkan_list;
    string final_henkan;
    istringstream ss(inputString);
    string word; // for storing each word
    // Traverse through all words
    // while loop till we get
    // strings to store in string word
    string temp = "";
    while (ss >> word)
    {
        temp += word + "";
        if (temp.size() >= LOAD_BALANCE) {
            henkan_list.emplace_back(temp);
            temp = "";
        }
    }
    if (temp != "") {
        henkan_list.emplace_back(temp);
    }
    vector<string> henkan_out = vector<string>(henkan_list.size());
    if (henkan_list.size() >= 2) {
         #pragma omp parallel
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < henkan_list.size(); i++) {
                int thread_num = omp_get_thread_num();
                // std::cout << thread_num << henkan_list[i] << std::endl;
                henkan_out[i] = edgeHenkanParallel(henkan_list[i]);
            }
        }
        final_henkan = boost::algorithm::join(henkan_out, "");
    } else {
        final_henkan =  edgeHenkanParallel(henkan_list[0]);
    }

   
    return final_henkan;
}

string edgeHenkan(string inputString){
    int frontBound, endBound, frontOffset, backOffset;
    frontBound = 0; endBound = inputString.length(); //pointers for input string
    string returnMe = "";
    frontOffset = 0; backOffset = 0; //pointers for output string
    if(inputString.size() < GRANULARITY_BOUND){
        //granularity bound
        return naiveHenkan(inputString);
    }
    while(frontBound < endBound){
        for (int w = 0; w < 2; w++) {
            if (w == 0) {
                int j = min(4,endBound-frontBound);
                while (j > 0) {
                    if(frontBound + 4 >= endBound){
                        break;
                    }
                    if(inDict(inputString.substr(endBound-j,j)) != NOT_FOUND) {
                        //cout << "found endBound " << inputString.substr(endBound-i,i) << std::endl; 
                        //cout << "inserting" << insertMe <<"\n" << std::endl;
                        //cout << "string: " <<returnMe << "int: " << (returnMe.length()-backOffset) << std::endl;
                        string insertMe = retrieveDict(inputString.substr(endBound-j,j));
                        returnMe.insert(returnMe.length()-backOffset,insertMe);
                        backOffset += insertMe.length(); 
                        endBound -= j;
                        // cout << "found back " << insertMe << endl;
                        break;
                    }
                    j--;
                }
            } else {
                for(int i = min(4,(int)inputString.length()/2); i > 0; i--){ //4 is maxsize romaji in dict
            //cout << "trying frontBound" << inputString.substr(frontBound,i) << std::endl;
                    if(frontBound >= endBound){
                        break;
                    }
                    if(inDict(inputString.substr(frontBound,i)) != NOT_FOUND) {        
                        //cout << "found frontBound" << inputString.substr(frontBound,i) << std::endl;
                        //cout << "inserting" << retrieveDict(inputString.substr(frontBound,i)) <<"\n" << std::endl;
                        string insertMe = retrieveDict(inputString.substr(frontBound,i));
                        returnMe.insert(frontOffset,insertMe);
                        frontOffset += insertMe.length();
                        frontBound += i;
                        // cout << "found front " << insertMe << endl;
                        break;
                    } else if (i == 1) {
                        // in case there are no matches possible, move up front in order to continue execution
                        returnMe.insert(frontOffset,inputString.substr(frontBound,i));
                        frontOffset += 1;
                        frontBound += 1;
                    }
                //cout << "trying endBound " << inputString.substr(endBound-i,i) << std::endl;

                //cout << "string = " << returnMe << "length = " << returnMe.length() << std::endl;
                } //might want to break up for loops to ensure that 1 is not stall waiting for other if already found

            }
        }
       
    }

    return returnMe;

}

string edgeHenkanParallel(string inputString){
    int sharedFrontBound, sharedEndBound, endBound, frontBound, frontOffset, backOffset;
    frontBound = 0; endBound = inputString.length(); //pointers for input string
    sharedFrontBound = frontBound;
    sharedEndBound = endBound;
    string left;
    string right;
    frontOffset = 0; backOffset = 0; //pointers for output string
    if(inputString.size() < GRANULARITY_BOUND){
        //granularity bound
        return naiveHenkan(inputString);
    }

        //#pragma omp parallel
        #pragma omp parallel num_threads(2)
        #pragma omp sections
        {

        #pragma omp section
        {
            while(frontBound < endBound){
                for(int i = min(4,(int)inputString.length()/2); i > 0; i--){ //4 is maxsize romaji in dict
                        // cout << "trying frontBound" << inputString.substr(frontBound,i) << " " << frontBound << " " << i << " " << omp_get_thread_num() << std::endl;
                        #pragma omp atomic read seq_cst
                        endBound = sharedEndBound;
                        if(frontBound >= endBound){
                            break;
                        }
                        if(inDict(inputString.substr(frontBound,i)) != NOT_FOUND) {        
                            //cout << "found frontBound" << inputString.substr(frontBound,i) << std::endl;
                            //cout << "inserting" << retrieveDict(inputString.substr(frontBound,i)) <<"\n" << std::endl;
                            string insertMe = retrieveDict(inputString.substr(frontBound,i));
                            left += insertMe;
                            frontOffset += insertMe.length();
                            frontBound += i;
                            #pragma omp atomic write seq_cst
                            sharedFrontBound = frontBound;
                            // cout << "found front " << insertMe << endl;
                            break;
                        } else if (i == 1) {
                            // in case there are no matches possible, move up front in order to continue execution
                            left += inputString.substr(frontBound,i);
                            frontOffset += 1;
                            frontBound += 1;
                        }
                    //cout << "trying endBound " << inputString.substr(endBound-i,i) << std::endl;

                    //cout << "string = " << returnMe << "length = " << returnMe.length() << std::endl;

                }
            }
        }
        #pragma omp section
        {
            while(frontBound < endBound){
                // std::cout << " hi " << omp_get_num_threads() << endl;
                int j = min(4,endBound-frontBound);
                while (j > 0) {
                    #pragma omp atomic read seq_cst
                    frontBound = sharedFrontBound;
                    if(frontBound + 4 >= endBound){
                        break;
                    }
                    if(inDict(inputString.substr(endBound-j,j)) != NOT_FOUND) {
                       
                        //cout << "found endBound " << inputString.substr(endBound-i,i) << std::endl; 
                        //cout << "inserting" << insertMe <<"\n" << std::endl;
                        //cout << "string: " <<returnMe << "int: " << (returnMe.length()-backOffset) << std::endl;
                        string insertMe = retrieveDict(inputString.substr(endBound-j,j));
                        right.insert(0,insertMe);
                        backOffset += insertMe.length(); 
                        endBound -= j;
                        #pragma omp atomic write seq_cst
                        sharedEndBound = endBound;
                        // cout << "found back " << insertMe << endl;
                        break;
                    }
                    j--;
                }
            }
        }
        }
        

 //might want to break up for loops to ensure that 1 is not stall waiting for other if already found


            // int thread_num = omp_get_thread_num();
            // std::cout << thread_num  << std::endl;

    return left + right;

}
void dut(string inputString, bool verbose){
    string naiveInput = inputString;
    naiveInput.erase(remove(naiveInput.begin(), naiveInput.end(), ' '), naiveInput.end());

    Timer naiveSimulationTimer;
    string naiveOutput = sequentialEdgeHenkan(naiveInput);
    double naiveTime = naiveSimulationTimer.elapsed();

    Timer edgeSimulationTimer;
    string edgeOutput = parallelEdgeHenkan(inputString);
    double edgeSimulationTime = edgeSimulationTimer.elapsed();

    Timer edgeSimNoEdgeParallelTimer;
    string edgeNoEdgeParallelOutput = parallelEdgeHenkanNoParallel(inputString);
    double edgeSimNoEdgeParallelTime = edgeSimNoEdgeParallelTimer.elapsed();

    naiveOutput.erase(remove(naiveOutput.begin(), naiveOutput.end(), ' '), naiveOutput.end());
    edgeOutput.erase(remove(edgeOutput.begin(), edgeOutput.end(), ' '), edgeOutput.end());
    edgeNoEdgeParallelOutput.erase(remove(edgeNoEdgeParallelOutput.begin(), edgeNoEdgeParallelOutput.end(), ' '), edgeNoEdgeParallelOutput.end());
    
    if (verbose == true) {
        cout << "naiveHenkan: " << naiveOutput << std::endl;
        cout << "edgeHenkan: " << edgeOutput << std::endl;
        cout << "edgeHenkanNoParallel: " << edgeNoEdgeParallelOutput << std::endl;
    }
    printf("sequential edgehenkan simulation time: %.6fs\n", naiveTime);
    printf("parallel edgeHenkan simulation time: %.6fs\n", edgeSimulationTime);
    if (verbose == true) {
        printf("parallel edgeHenkan noParallelEdge simulation time: %.6fs\n", edgeSimNoEdgeParallelTime);
    }
    printf("Speedup: %f\n", (naiveTime/(edgeSimulationTime)));
    if (verbose == true) {
        printf("parallel edgeHenkan noParallelEdge over sequential speedup:  %f\n", (naiveTime/edgeSimNoEdgeParallelTime));
    }
    //insert timing code

    if(naiveOutput == edgeOutput){
        cout << "\033[37;32mOUTPUTS MATCH!" << std::endl;
    }else{
        cout << "\033[37;31mOUTPUTS DO NOT MATCH :(" << std::endl;
    }
}

int main(int argc, const char **argv) {
    string inputString;
    bool verbose = false;
    for (int i = 1; i < argc; i++){
        if (i < argc - 1) {
            if (strcmp(argv[i], "-in") == 0) {
                inputString = read_input_file((string)argv[i+1]);
            }
        }
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
    } 
    if (inputString == "") {
        cout << "Please Enter an input" << std::endl;
        getline(cin, inputString);
    }
    boost::algorithm::to_lower(inputString);
    read_hira();
    dut(inputString, verbose);

    return 0;
}

// eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen

