/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamguprimemapfu.h
 Date       : 2017/09/01
 Purpose    : map implementation 
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#ifndef Tamguprimemapfu_h
#define Tamguprimemapfu_h

#include "fractalhmap.h"
#include "tamguint.h"
#include "tamgustring.h"
#include "tamguvector.h"
#include "tamguconstants.h"
#include "tamgufvector.h"
#include "tamguuvector.h"

//We create a map between our methods, which have been declared in our class below. See MethodInitialization for an example
//of how to declare a new method.
class Tamguprimemapfu;
//This typedef defines a type "primemapfuMethod", which expose the typical parameters of a new Tamgu method implementation
typedef Tamgu* (Tamguprimemapfu::*primemapfuMethod)(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
//---------------------------------------------------------------------------------------------------------------------
class Tamguprimemapfu : public TamguLockContainer {
    public:
    //We export the methods that will be exposed for our new object
    //this is a static object, which is common to everyone
    //We associate the method pointers with their names in the linkedmethods map
    static Exchanging basebin_hash<primemapfuMethod> methods;
    static Exchanging hmap<string, string> infomethods;
    static Exchanging  basebin_hash<unsigned long> exported;

    static Exchanging short idtype;

    //---------------------------------------------------------------------------------------------------------------------
    //This SECTION is for your specific implementation...
    //Your personal variables here...
    prime_hash<double, std::wstring> values;
    bool isconst;

    //---------------------------------------------------------------------------------------------------------------------
    Tamguprimemapfu(TamguGlobal* g, Tamgu* parent = NULL) : TamguLockContainer(g, parent) {
     investigate |= is_string;
        //Do not forget your variable initialisation
        isconst = false; 

    }

    Tamguprimemapfu() {
     investigate |= is_string;
        //Do not forget your variable initialisation
        isconst = false; 

    }

    //----------------------------------------------------------------------------------------------------------------------
    Exporting Tamgu* Loopin(TamguInstruction* ins, Tamgu* context, short idthread);
    Exporting Tamgu* Put(Tamgu* index, Tamgu* value, short idthread);
    Exporting Tamgu* Eval(Tamgu* context, Tamgu* value, short idthread);

    void SetConst() { isconst = true;}

    short Type() {
        return Tamguprimemapfu::idtype;
    }

    string Typename() {
        return "primemapfu";
    }

    

    bool isContainerClass() {
        return true;
    }

    bool isMapContainer() {
        return true;
    }

    bool isValueContainer() {
        return true;
    }


    Tamgu* Atom(bool forced) {
        if (forced || !protect || reference) {
            locking();
            Tamguprimemapfu * m = new Tamguprimemapfu;
            m->values = values;
            unlocking();
            return m;
        }
        return this;
    }
    
    //---------------------------------------------------------------------------------------------------------------------
    //Declaration
    //All our methods must have been declared in tamguexportedmethods... See MethodInitialization below
    bool isDeclared(short n) {
        if (exported.find(n) != exported.end())
            return true;
        return false;
    }

    
    Tamgu* Newvalue(Tamgu* a, short idthread) {
        Tamguprimemapfu* m = new Tamguprimemapfu;
        if (a->isContainer()) {
            TamguIteration* it = a->Newiteration(false);
            for (it->Begin(); it->End() == aFALSE; it->Next()) {
                m->values[it->Keyfloat()]=it->Valueustring();
            }
            it->Release();
            return m;
        }
        
        wstring val=a->UString();
        prime_hash<double,wstring>::iterator ist;
        for (ist=values.begin(); ist!=values.end();ist++)
            m->values[ist->first]=val;
        return m;
    }

    Tamgu* Newinstance(short idthread, Tamgu* f = NULL) {
        return new Tamguprimemapfu();
    }

    Exporting TamguIteration* Newiteration(bool direction);

    bool duplicateForCall() {
        return isconst;
    }




    static void AddMethod(TamguGlobal* global, string name, primemapfuMethod func, unsigned long arity, string infos);
    static bool InitialisationModule(TamguGlobal* global, string version);

    void Methods(Tamgu* v) {
        hmap<string, string>::iterator it;
        for (it = infomethods.begin(); it != infomethods.end(); it++)
            v->storevalue(it->first);
    }

    string Info(string n) {

        if (infomethods.find(n) != infomethods.end())
            return infomethods[n];
        return "Unknown method";
    }



    //---------------------------------------------------------------------------------------------------------------------
    //This SECTION is for your specific implementation...
    //This is an example of a function that could be implemented for your needs.

    
    Tamgu*
    MethodInvert(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        if (contextualpattern == this || !contextualpattern->isMapContainer() || !contextualpattern->isAffectation())
            contextualpattern = Newinstance(idthread);
        else
            contextualpattern->Clear();

        Tamgu* a;
        prime_hash<double, wstring>::iterator it;
        for (it = values.begin(); it != values.end(); it++) {
            a = globalTamgu->Providefloat(it->first);
            contextualpattern->Push(it->second, a);
            a->Release();
        }

        return contextualpattern;
    }

    Exporting Tamgu* MethodFind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);


    Tamgu* MethodClear(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Clear();
        return aTRUE;
    }


    
    Tamgu* MethodItems(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        prime_hash<double, wstring>::iterator itr;
        Tamguprimemapfu* item;
        Tamgu* vect = Selectavector(contextualpattern);
        for (itr=values.begin(); itr!=values.end();itr++) {
            item=new Tamguprimemapfu;
            item->values[itr->first] = itr->second;
            vect->Push(item);
        }
        return vect;
    }
    
    Tamgu* MethodMerge(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {

        
        Tamgu* v = callfunc->Evaluate(0, contextualpattern, idthread);
        Merging(v);
        return this;
    }

    Tamgu* MethodKeys(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        locking();
        Tamgufvector* vstr = (Tamgufvector*)Selectafvector(contextualpattern);
        prime_hash<double, wstring>::iterator it;
        for (it = values.begin(); it != values.end(); it++)
            vstr->values.push_back(it->first);
        unlocking();
        return vstr;
    }

    Tamgu* MethodValues(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        locking();
        Tamguuvector * vstr = (Tamguuvector*)Selectauvector(contextualpattern);
        prime_hash<double, wstring>::iterator it;
        for (it = values.begin(); it != values.end(); it++)
            vstr->values.push_back(it->second);
        unlocking();
        return vstr;
    }

    Tamgu* MethodTest(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        locking();
        double  v = callfunc->Evaluate(0, contextualpattern, idthread)->Float();
        if (values.find(v) == values.end()) {
            unlocking();
            return aFALSE;
        }
        unlocking();
        return aTRUE;
    }

    Tamgu* MethodPop(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamgu* pos = callfunc->Evaluate(0, contextualpattern, idthread);
        return Pop(pos);
    }

    Tamgu* MethodJoin(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        locking();
        //The separator between keys
        wstring keysep = callfunc->Evaluate(0, contextualpattern, idthread)->UString();
        //The separator between values
        wstring sep = callfunc->Evaluate(1, contextualpattern, idthread)->UString();
        prime_hash<double, wstring>::iterator it;
        bool beg = true;
        std::wstringstream res;
        for (it = values.begin(); it != values.end(); it++) {
            if (beg == false)
                res << sep;
            beg = false;
            res << it->first << keysep << it->second;
        }

        unlocking();
        return globalTamgu->Provideustring(res.str());
    }


    //------------------------------------------------------------------------------------------
    Exporting Tamgu* Push(Tamgu* k, Tamgu* v);
    Exporting Tamgu* Pop(Tamgu* kkey);

    Tamgu* Push(double k, Tamgu* a) {
        locking();
        values[k] = a->UString();
        unlocking();
        return this;
    }

    Tamgu* Combine(Tamgu* ke) {
        //Three cases:
        if (!ke->isContainer())
            return this;
        Doublelocking(this, ke);


        TamguIteration* itr = ke->Newiteration(false);
        for (itr->Begin(); itr->End() == aFALSE; itr->Next()) {
            double n = itr->Keyfloat();
            if (values.find(n) == values.end())
                values[n] = itr->Valueustring();
            else
                values[n] += itr->Valueustring();
        }
        itr->Release();
        return this;
    }

    Tamgu* Merging(Tamgu* ke) {
        //Three cases:
        if (!ke->isContainer())
            return this;
        Doublelocking(this, ke);


        TamguIteration* itr = ke->Newiteration(false);
        for (itr->Begin(); itr->End() == aFALSE; itr->Next()) {
            double n = itr->Keyfloat();
            if (values.find(n) == values.end())
                values[n] = itr->Valueustring();
        }
        itr->Release();
        return this;
    }

    //---------------------------------------------------------------------------------------------------------------------

    //ExecuteMethod must be implemented in order to execute our new Tamgu methods. This method is called when a TamguCallMethodMethod object
    //is returned by the Declaration method.
    Tamgu* CallMethod(short idname, Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        //This call is a bit cryptic. It takes the method (function) pointer that has been associated in our map with "name"
        //and run it with the proper parameters. This is the right call which should be invoked from within a class definition
        return (this->*methods.get(idname))(contextualpattern, idthread, callfunc);
    }

    Exporting void Clear();
    

    Exporting string String();
    Exporting string JSonString();

    Tamgu* Value(string& n) {
        double v = convertdouble(n);
        locking();
        if (values.find(v) == values.end()) {
            unlocking();
            return aNOELEMENT;
        }
        Tamgu* res = globalTamgu->Provideustring(values[v]);
        unlocking();
        return res;        
    }

    Tamgu* Value(long n) {
        locking();
        if (values.find((double)n) == values.end()) {
            unlocking();
            return aNOELEMENT;
        }
        Tamgu* res = globalTamgu->Provideustring(values[(double)n]);
        unlocking();
        return res;        
    }

    Tamgu* Value(Tamgu* a) {
        double n =  a->Float();

        locking();
        if (values.find((double)n) == values.end()) {
            unlocking();
            return aNOELEMENT;
        }
        Tamgu* res = globalTamgu->Provideustring(values[(double)n]);
        unlocking();
        return res;        
    }

    Tamgu* Value(double n) {
        locking();
        if (values.find((double)n) == values.end()) {
            unlocking();
            return aNOELEMENT;
        }
        Tamgu* res = globalTamgu->Provideustring(values[(double)n]);
        unlocking();
        return res;        
    }

    Exporting long Integer();
    Exporting double Float();
    Exporting BLONG Long();
    Exporting bool Boolean();


    //Basic operations
    Exporting long Size();
    
    Exporting Tamgu* in(Tamgu* context, Tamgu* a, short idthread);

    Exporting Tamgu* andset(Tamgu* a, bool itself);
    Exporting Tamgu* orset(Tamgu* a, bool itself);
    Exporting Tamgu* xorset(Tamgu* a, bool itself);
    Exporting Tamgu* plus(Tamgu* a, bool itself);
    Exporting Tamgu* minus(Tamgu* a, bool itself);

    Exporting Tamgu* same(Tamgu* a);

};

//--------------------------------------------------------------------------------------------------
class TamguIterationprimemapfu : public TamguIteration {
    public:

    prime_hash<double, wstring>::iterator it;
    Tamguprimemapfu map;

    TamguIterationprimemapfu(Tamguprimemapfu* m, bool d, TamguGlobal* g = NULL) : TamguIteration(d, g) {
        map.values = m->values;
    }

    
    void Setvalue(Tamgu* recipient, short idthread) {
        recipient->storevalue(it->first);
    }

    Tamgu* Key() {
        return globalTamgu->Providefloat(it->first);
    }

    
    

    Tamgu* Value() {
        return globalTamgu->Provideustring(it->second);
    }

    string Keystring() {
        
        return convertfromnumber(it->first);
        
    }

    
    BLONG Keylong() {
        return it->first;
    }

    long Keyinteger() {
        return it->first;
    }

    double Keyfloat() {
        return it->first;
    }

    wstring Valueustring() {
        return it->second;
    }

    string Valuestring() {
        string res;
        wstring w = it->second;
        s_unicode_to_utf8(res, w);
        return res;
    }

    long Valueinteger() {
        return convertinteger(it->second);
    }

    double Valuefloat() {
        return convertfloat(it->second);
    }

    void Next() {
        it++;
    }

    Tamgu* End() {
        if (it == map.values.end())
            return aTRUE;
        return aFALSE;
    }

    Tamgu* Begin() {
        it = map.values.begin();
        return aTRUE;
    }

};


//-------------------------------------------------------------------------------------------

#endif
