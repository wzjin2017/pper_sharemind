// @file  simple-integers.cpp - Simple example for BFVrns (integer arithmetic).
// @author TPOC: contact@palisade-crypto.org
//
// @copyright Copyright (c) 2019, New Jersey Institute of Technology (NJIT))
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution. THIS SOFTWARE IS
// PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "palisade.h"
#include <iostream>
#include <fstream>
#include <map>
#include <time.h>
#include <stdlib.h>

using namespace std;
using namespace lbcrypto;

vector<vector<string>> readFromCSVFile(string csvFilePath);
vector<string> tokenHelper(int n, string record);
map<string, vector<string>> tokenizeRecords(vector<vector<string>> set);
map<string, vector<int64_t>> preProcessPlaintexts(
    map<string, vector<string>> set);

int main() {
  int plaintextModulus = 65537;
  double sigma = 3.2;
  SecurityLevel securityLevel = HEStd_128_classic;
  uint32_t depth = 2;

  // Instantiate the crypto context
  CryptoContext<DCRTPoly> cc =
      CryptoContextFactory<DCRTPoly>::genCryptoContextBFVrns(
          plaintextModulus, securityLevel, sigma, 0, depth, 0, OPTIMIZED);

  // Enable features that to use
  cc->Enable(ENCRYPTION);
  cc->Enable(SHE);
  cc->Enable(MULTIPARTY);

  //  vector<vector<string>> setA = readFromCSVFile(
  //      "/Users/tanmay.ghai/Desktop/palisade-development/src/pke/examples/"
  //      "gen-1k_300-700-5-5-5-zipf-all_200.csv");
  //  vector<vector<string>> setB = readFromCSVFile(
  //      "/Users/tanmay.ghai/Desktop/palisade-development/src/pke/examples/"
  //      "gen-1k_300-700-5-5-5-zipf-all_800.csv");
  //
  //  map<string, vector<string>> a = tokenizeRecords(setA);
  //  map<string, vector<string>> b = tokenizeRecords(setB);
  //
  //  map<string, vector<int64_t>> plaintextsA = preProcessPlaintexts(a);
  //  map<string, vector<int64_t>> plaintextsB = preProcessPlaintexts(b);
  //
  //
  // Initialize Public Key Containers
  LPKeyPair<DCRTPoly> keyPair;

  // Generate a public/private key pair
  keyPair = cc->KeyGen();

  cc->EvalMultKeyGen(keyPair.secretKey);


  //
  //  vector<Ciphertext<DCRTPoly>> ciphertexts;
  //  map<string, vector<int64_t>>::iterator it;
  //  for (it = plaintextsB.begin(); it != plaintextsB.end(); it++) {
  //    string key = it->first;
  //    Plaintext plaintext = cc->MakePackedPlaintext(it->second);
  //    Ciphertext<DCRTPoly> ciphertext = cc->Encrypt(keyPair.publicKey,
  //    plaintext); ciphertexts.push_back(ciphertext);
  //  }

  // set X
  vector<int64_t> setX = {3,2,1,4};
  // set Y
  vector<int64_t> setY = {1,2,3,5};

  vector<Plaintext> pX;
  vector<Plaintext> pY;

  // generating simple integer plaintexts for setX
  for (int i = 0; i < setX.size(); i++) {
    pX.push_back(cc->MakeIntegerPlaintext(setX[i]));
  }

  // generating simple integer plaintexts for setY
  for (int i = 0; i < setY.size(); i++) {
    pY.push_back(cc->MakeIntegerPlaintext(setY[i]));
  }

  // encrypting contents of set Y
  vector<Ciphertext<DCRTPoly>> ciphertexts;
  for (int i = 0; i < pY.size(); i++) {
    ciphertexts.push_back(cc->Encrypt(keyPair.publicKey, pY[i]));
  }


  // implementing Basic PSI protocol from: https://eprint.iacr.org/2017/299.pdf
  vector<Ciphertext<DCRTPoly>> returnCiphertexts;
  for (int i = 0; i < ciphertexts.size(); i++) {
    // sample random non-zero plaintext
    Plaintext r = cc->MakeIntegerPlaintext((rand() % 100) + 1);
    // initializing d_i to be product of difference between each c_i and each x in X
    auto sub1 = cc->EvalSub(ciphertexts[i], pX[0]);
    auto sub2 = cc->EvalSub(ciphertexts[i], pX[1]);
    auto d = cc->EvalMult(sub1, sub2);
    for (int j = 2; j < pX.size(); j++) {
      d = cc->EvalMult(d, cc->EvalSub(ciphertexts[i], pX[j]));
    }
    // decrypting the ciphertexts; if y+i : FHE.decrypt(d_i) = 0, then we know
    // that y_i exists in X and is part of the set intersection.
    returnCiphertexts.push_back(cc->EvalMult(r, d));
  }

  vector<int64_t> setIntersectionResult;
  for (int i = 0; i < returnCiphertexts.size(); i++) {
    Plaintext decryptResult;
    // decrypting the ciphertexts; if y+i : FHE.decrypt(d_i) = 0, then we know
    // that y_i exists in X and is part of the set intersection.
    cc->Decrypt(keyPair.secretKey, returnCiphertexts[i], &decryptResult);
    if (decryptResult->GetIntegerValue() == 0) {
      setIntersectionResult.push_back(setY[i]);
    }
  }
  cout << "set X: " << setX << endl;
  cout << "set Y: " << setY << endl;
  cout << "Set Intersection between sets X and Y: " << setIntersectionResult << endl;

  return 0;
}

/*
 * preProcessPlaintexts(map<string, vector<string>>): convert each vector of
 * n-grams into a vector of int64_t's to prepare for encryption with Palisades
 * functions.
 */
map<string, vector<int64_t>> preProcessPlaintexts(
    map<string, vector<string>> set) {
  map<string, vector<int64_t>> plaintexts;
  map<string, vector<string>>::iterator it;
  for (it = set.begin(); it != set.end(); it++) {
    vector<int64_t> plaintext;
    string key = it->first;
    vector<string> v = it->second;
    for (string s : v) {
      for (char c : s) {
        plaintext.push_back(int(c));
      }
    }
    plaintexts[key] = plaintext;
  }
  return plaintexts;
}

/*
 * tokenHelper(int n, string record): helper method for the tokenization
 * process, converting a string to a n-gram, with n being a parameter.
 */
vector<string> tokenHelper(int n, string record) {
  vector<string> tokenized;
  if (record.size() == 0) {
    return tokenized;
  }

  for (int i = 0; i < record.size() - n + 1; i++) {
    tokenized.push_back(record.substr(i, n));
  }

  return tokenized;
}

/*
 * tokenizeRecords(vector<vector<string>> set): converts each vector of strings
 * into a tokenized set of n-grams. For my purposes here, I chose bi-grams.
 * ex: ["tanmay"] -> ["ta", "an", "nm", "ma", ay"]
 */
map<string, vector<string>> tokenizeRecords(vector<vector<string>> set) {
  map<string, vector<string>> tokenSet;
  for (int i = 0; i < set.size(); i++) {
    string key = set[i][0];
    set[i].erase(set[i].begin());
    set[i].erase(set[i].end() - 1);

    string s;
    for (const auto &piece : set[i]) s += piece;
    tokenSet.insert(pair<string, vector<string>>(key, tokenHelper(2, s)));
  }
  return tokenSet;
}
/*
 * readFromCSVFile(string csvFilePath): reads in the two csv files, each
 * second-level vector is a single line in the csv file where each string in the
 * vector is a comma separated field in the input.
 */
vector<vector<string>> readFromCSVFile(string csvFilePath) {
  ifstream setA;
  setA.open(csvFilePath);

  if (!setA.is_open() || setA.fail()) {
    std::cout << "ERROR: File could not be opened" << '\n';
  }

  // std::initializer_list<string> recordKeys = {rec_id, culture, sex, age,
  // date_of_birth, title, given_name, surname, state, suburb, postcode,
  // street_number, address_1, address_2, phone_number,soc_sec_id,
  // blocking_number, family_role, _gt_id};

  vector<vector<string>> records;
  string line;
  while (getline(setA, line)) {
    stringstream ss(line);
    vector<string> row;
    string data;
    while (getline(ss, data, ',')) {
      row.push_back(data);
    }
    records.push_back(row);
  }
  setA.close();
  return records;
}
