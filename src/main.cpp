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

//tweaking parameters
int GRANULARITY_BOUND = 4;
int LOAD_BALANCE = 200;

//enum for condition codes for querying dictionary/exceptions
enum dictStatus {FOUND_DICT, FOUND_EXCEPTION, NOT_FOUND};

//romaji-to-Hiragana hashmap
std::map<std::string, std::string> romajiHiraganaMap;

string edgeHenkan(string inputString);
string edgeHenkanParallel(string inputString);

// Helper function to read input file and return as a string
string read_input_file(string inputSrc) {
    ifstream file;
    file.open(inputSrc); //open the input file
    if (!file.is_open())
    {
        cerr << "Input File Path is Invalid" << endl;
        return "";
    }
    stringstream strStream;
    strStream << file.rdbuf(); //read the file
    string file_string = strStream.str();

    return file_string;
}

// Helper function to read in hiragana mappings from spreadsheet
void read_hira() {
    string line, word, romaji, hiragana;
    ifstream file;

    file.open("hiraganaMap.csv");

    if (!file.is_open())
    {
        cout << "Input File Path is Invalid" << endl;
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

/// @brief Function to declare all the romaji Exceptions not found in the dictionary
/// @param queryString 
/// @return Converted character if an exception is found, returns emptry string otherwise
string romajiExceptions(string queryString){
    //space
    if (queryString == " "){
        return " ";
    }
    //little tsu exception (size 4 case)
    else if(queryString.size() == 4 && queryString[0] == queryString[1] && romajiHiraganaMap.find(queryString.substr(1,3)) != romajiHiraganaMap.end()){
        return "っ" + romajiHiraganaMap[queryString.substr(1,3)];
    }
    //little tsu exception (size 3 case)
    else if(queryString.size() == 3 && queryString[0] == queryString[1] && romajiHiraganaMap.find(queryString.substr(1,2)) != romajiHiraganaMap.end()){
        return "っ" + romajiHiraganaMap[queryString.substr(1,2)];
    }
    //hyphen (nobashibo) exception
    else if(queryString == "-"){
        return "ー";
    }
    //non-alphabet exception (i.e. numbers, special characters like $ @ %)
    else if(queryString.size() == 1 && ((queryString[0] < 'a' || queryString[0] > 'z') && (queryString[0] < 'A' || queryString[0] > 'Z'))){
        return queryString;
    }
    //not an exception (return the empty string)
    return "";
}

/// @brief Function to check if a queryString has a mapping
/// @param queryString 
/// @return FOUND_DICT if queryString was found in the hashmap, 
///         FOUND_EXCEPTION if found as an exception, NOT_FOUND if not found in either
dictStatus inDict(string queryString){
    if((romajiHiraganaMap.find(queryString) != romajiHiraganaMap.end())){
        //found in hashmap
        return FOUND_DICT;
    }
    else if (romajiExceptions(queryString) != ""){
        //found in exceptions
        return FOUND_EXCEPTION;
    }
    //not found
    return NOT_FOUND;
}

/// @brief Function to retrieve a queryString mapping
/// @param queryString 
/// @return queryString mapped to its Hiragana (Japanese character) equivalent
// NOTE: All calls to this function have the assertion that inDict != NOT_FOUND
string retrieveDict(string queryString){
    dictStatus qs = inDict(queryString);
    if(qs == FOUND_DICT){
        return romajiHiraganaMap[queryString];
    }
    else if(qs == FOUND_EXCEPTION){
        return romajiExceptions(queryString);
    }
    //should not hit this case
    return "NF";
}

//naiveHenkan:
//Sequentially converts the inputString from left to right using recursion.

/// @brief Sequentially converts the inputString from left to right using recursion
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
string naiveHenkan(string inputString){
    if(inputString.size() == 0){
        //base case
        return "";
    }
    for(size_t i = inputString.size() ; i >0 ; i--){
        if(inDict(inputString.substr(0,i)) != NOT_FOUND) {
            // mapping found
            string recursiveCall = naiveHenkan(inputString.substr(i,inputString.size()-i));
            return retrieveDict(inputString.substr(0,i)) + recursiveCall;
        }
    }
    return inputString;
}
/// @brief Sequentially converts the inputstring from both ends of the input string
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
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
/// @brief Load-balanced parallel implementation that relies on
///        that relies on per-thread sequential processing (details in writeup)
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
string parallelEdgeHenkanV2(string inputString) {
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
         #pragma omp parallel num_threads(8)
        {
            #pragma omp for schedule(static)
            for (int i = 0; i < henkan_list.size(); i++) {
                int thread_num = omp_get_num_threads();
                henkan_out[i] = edgeHenkan(henkan_list[i]);
            }
        }
        final_henkan = boost::algorithm::join(henkan_out, "");
    } 
    else {
        final_henkan =  edgeHenkan(henkan_list[0]);
    }
    return final_henkan;
}

/// @brief Parallel version of edgeHenkan version 1 (more details in writeup)
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
string parallelEdgeHenkanV1(string inputString) {
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
    } 
    else {
        final_henkan =  edgeHenkanParallel(henkan_list[0]);
    }
    return final_henkan;
}

/// @brief edgeHenkan implementation without any OpenMP threads nor load-balancing
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
inline string edgeHenkan(string inputString){
    int frontBound, endBound, frontOffset, backOffset;
    frontBound = 0; endBound = inputString.length(); //pointers for input string
    string returnMe = "";
    frontOffset = 0; backOffset = 0; //pointers for output string
    if(inputString.size() < GRANULARITY_BOUND){
        //Granularity bound
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
                        string insertMe = retrieveDict(inputString.substr(endBound-j,j));
                        returnMe.insert(returnMe.length()-backOffset,insertMe);
                        backOffset += insertMe.length(); 
                        endBound -= j;
                        break;
                    }
                    j--;
                }
            } 
            else {
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
                        break;
                    } else if (i == 1) {
                        // in case there are no matches possible, move up front in order to continue execution
                        returnMe.insert(frontOffset,inputString.substr(frontBound,i));
                        frontOffset += 1;
                        frontBound += 1;
                    }
                }   
            }      
        }
    }
    return returnMe;
}
/// @brief edgeHenkan implementation with two-thread OpenMP parallelism
/// @param inputString a english character romaji input
/// @return outputString, a hiragana mapping of the inputString
string edgeHenkanParallel(string inputString){
    int sharedFrontBound, sharedEndBound, endBound, frontBound, frontOffset, backOffset;
    string left, right;

    frontBound = 0; endBound = inputString.length(); //pointers for input string
    frontOffset = 0; backOffset = 0; //pointers for output string
    sharedFrontBound = frontBound;
    sharedEndBound = endBound;

    if(inputString.size() < GRANULARITY_BOUND){
        //Granularity bound
        return naiveHenkan(inputString);
    }

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
    return left + right;
}
//Helper function to run, time, and calculate speedup for all the functions under testing
void dut(string inputString, bool verbose){
    string naiveInput = inputString;
    naiveInput.erase(remove(naiveInput.begin(), naiveInput.end(), ' '), naiveInput.end());

    //naive Implementation timing
    Timer naiveSimulationTimer;
    string naiveOutput = naiveHenkan(naiveInput);
    double naiveTime = naiveSimulationTimer.elapsed();

    //sequentialEdgeHenkan Implementation timing
    Timer seqSimulationTimer;
    string seqOutput = sequentialEdgeHenkan(naiveInput);
    double seqTime = seqSimulationTimer.elapsed();

    //parallelEdgeHenkanV1 Implementation timing
    Timer edgeSimulationTimer;
    string edgeOutput = parallelEdgeHenkanV1(inputString);
    double edgeSimulationTime = edgeSimulationTimer.elapsed();

    //parallelEdgeHenkanV2 Implementation timing
    Timer edgeSimNoEdgeParallelTimer;
    string edgeNoEdgeParallelOutput = parallelEdgeHenkanV2(inputString);
    double edgeSimNoEdgeParallelTime = edgeSimNoEdgeParallelTimer.elapsed();

    //erase spaces from outputs
    naiveOutput.erase(remove(naiveOutput.begin(), naiveOutput.end(), ' '), naiveOutput.end());
    edgeOutput.erase(remove(edgeOutput.begin(), edgeOutput.end(), ' '), edgeOutput.end());
    edgeNoEdgeParallelOutput.erase(remove(edgeNoEdgeParallelOutput.begin(), edgeNoEdgeParallelOutput.end(), ' '), edgeNoEdgeParallelOutput.end());
    
    // print out outputs of conversion
    if (verbose) {
        //cout << "naiveHenkan: " << naiveOutput << std::endl;
        //cout << "edgeHenkan: " << seqOutput << std::endl;
        cout << "edgeHenkanNoParallel: " << edgeNoEdgeParallelOutput << std::endl;
    }
    
    // print results of timing
    printf("naiveHenkan simulation time: %.6fs\n", naiveTime);
    printf("parallelEdgeHenkanV1 simulation time: %.6fs\n", seqTime);
    printf("parallelEdgeHenkanV2 simulation time: %.6fs\n", edgeSimulationTime);

    if (verbose) {
        printf("parallel edgeHenkan noParallelEdge simulation time: %.6fs\n", edgeSimNoEdgeParallelTime);
    }
    printf("Speedup of sequential edgeHenkan vs. parallelEdgeHenkan over words: %f\n", (seqTime/(edgeSimulationTime)));
    printf("Speedup of naive edgeHenkan vs. parallelEdgeHenkan over words: %f\n", (naiveTime/(edgeSimulationTime)));
    if (verbose) {
        printf("parallel over words, no per-word, over sequential speedup:  %f\n", (seqTime/edgeSimNoEdgeParallelTime));
        printf("parallel over words, no per-word, over naive speedup:  %f\n", (naiveTime/edgeSimNoEdgeParallelTime));
    }

    //Verify correctness of results
    if(seqOutput == edgeNoEdgeParallelOutput){
        cout << "\033[37;32mOUTPUTS MATCH!" << std::endl;
    }else{
        cout << "\033[37;31mOUTPUTS DO NOT MATCH :(" << std::endl;
    }
}

//Main Function
//Is the body function for the entire program.
//Parses command-line inputs, calls functions to run and analyze algorithm
int main(int argc, const char **argv) {
    string inputString;
    bool verbose = false;
    //parse command line flags
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
    boost::algorithm::to_lower(inputString); //convert input string into all lower case
    read_hira(); //generate hashmap
    dut(inputString, verbose); //run and analyze algorithms

    return 0;
}

// eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen eigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimaseneigo wakarimasen eigo wakarimasen Ajia Amerikajin eigo wakarimasen

