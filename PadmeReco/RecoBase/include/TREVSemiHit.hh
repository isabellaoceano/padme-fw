/**
 * \file TREVSemiHit.hh
 * \author Georgi Georgiev <ggeorge@cern.ch>
 * \date 2016-07-07
 */

#include<vector>

#include"TREVDigi.hh"
#include"TREDigiVSampling.hh"
#include"TREDigiVDiscret.hh"
#include"fwTSimpleList.hh"

#ifndef _TREVSemiHit_hh_
#define _TREVSemiHit_hh_ 0
class TREVSemiHit{
  public:
  /// Add Digis related to this SemiHit
    void AddDigi(TREVDigi *digi);
  protected:
    typedef typename fw::TSimpleList<TREVDigi*> digiCont_t;
    digiCont_t  fAssocDigis;


};
#endif 
