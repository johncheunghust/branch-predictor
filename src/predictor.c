//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

uint32_t gmask;
uint32_t gsize;

// Gshare: global history based on index sharing
uint32_t ghr; 
uint8_t *bht; // BHT: key = index, val = SN, WN, WT, ST

// Tournament
uint8_t *choice; // l / g
uint8_t *localPredictors;
uint32_t *lht; // local history table
uint32_t gpred_cnt;


//
//TODO: Add your own Branch Predictor data structures here
//


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void gshare_init();
void tournament_init();

// Initialize the predictor
//
void
init_predictor()
{
  gsize = 1 << ghistoryBits;
  gmask = gsize - 1;

  if(bpType == GSHARE) gshare_init();
  if(bpType == TOURNAMENT) tournament_init();
}

void gshare_init() {
  ghr = 0;
  bht = (uint8_t *)malloc(sizeof(uint8_t) * gsize);
  memset(bht, 1, gsize);
}

void tournament_init() {
  uint32_t lsize = 1 << lhistoryBits;

  ghr = 0;
  gpred_cnt = 0;

  bht = (uint8_t *) malloc(sizeof(uint8_t) * gsize);
  memset(bht, 1, gsize); // initialize bht as weakly not taken

  choice = (uint8_t *) malloc(sizeof(uint8_t) * gsize);
  memset(choice, 2, gsize); // initialize global predictor

  localPredictors = (uint8_t *) malloc(sizeof(uint8_t) * lsize);
  memset(localPredictors, 1, lsize); // initialize local predictors as weakly not taken

  lht = (uint32_t *) calloc(1 << pcIndexBits, sizeof(uint32_t));
}
// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

uint8_t gshare_prediction(uint32_t pc) {
  uint32_t idx = (pc ^ ghr) & gmask;
  return bht[idx] >> (CNTER - 1) ? TAKEN : NOTTAKEN;
}

uint8_t tournament_prediction(uint32_t pc) {
  uint8_t res = NOTTAKEN;
  uint8_t which = choice[ghr] >> (CNTER - 1);
  if(which) {
    res = bht[ghr];
    gpred_cnt++;
  } else {
    res = localPredictors[lht[pc & ((1 << pcIndexBits) - 1)]];
  }
  return res >> (CNTER - 1) ? TAKEN : NOTTAKEN;
}

uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_prediction(pc);
    case TOURNAMENT:
      return tournament_prediction(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

void gshare_train(uint32_t pc, uint8_t outcome) {
  uint32_t idx = (pc ^ ghr) & gmask;
  ghr = (ghr << 1 | outcome) & gmask;
  if (outcome) {
    // the result TAKEN
    bht[idx] += bht[idx] == (1 << CNTER) - 1 ? 0 : 1; // Only when it is strongly taken we do not change the state
  } else {
    bht[idx] -= bht[idx] == 0 ? 0 : 1;
  }
}

void tournament_train(uint32_t pc, uint8_t outcome) {
  uint8_t CNTERCAP = (1 << CNTER) - 1;
  uint32_t pcIndex = pc & ((1 << pcIndexBits) -1);
  uint32_t lIndex = lht[pcIndex];

  uint8_t gCorrect = bht[ghr] >> (CNTER - 1) == outcome;
  uint8_t lCorrect = localPredictors[lIndex] >> (CNTER - 1) == outcome;

  int32_t act = gCorrect - lCorrect;
  if(act > 0) choice[ghr] += choice[ghr] == CNTERCAP ? 0 : 1;
  else if(act<0) choice[ghr] -= choice[ghr] == 0 ? 0 : 1;

  if(outcome) bht[ghr] += bht[ghr] == CNTERCAP ? 0 : 1;
  else bht[ghr] -= bht[ghr] == 0? 0 : 1;

  if(outcome) localPredictors[lIndex] += localPredictors[lIndex] == CNTERCAP ? 0 : 1;
  else localPredictors[lIndex] += localPredictors[lIndex] == CNTERCAP ? 0 : 1;

  ghr = (ghr << 1 | outcome) & gmask;
  lht[pcIndex] = (lht[pcIndex] << 1 | outcome) & ((1 << lhistoryBits) - 1);
}
// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  if(bpType == GSHARE) return gshare_train(pc, outcome); 
  else if(bpType == TOURNAMENT) return tournament_train(pc, outcome);
}
