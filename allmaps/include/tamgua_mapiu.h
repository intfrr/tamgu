/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamgua_mapiu.h
 Date       : 2017/09/01
 Purpose    : map implementation lockfree 
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#ifndef Tamgua_mapiu_h
#define Tamgua_mapiu_h

#include "atomicmap.h"
#include "tamguint.h"
#include "tamgustring.h"
#include "tamguvector.h"
#include "tamguconstants.h"
#include "tamguivector.h"
#include "tamguuvector.h"
#include "tamguivector.h"

//We create a map between our methods, which have been declared in our class below. See MethodInitialization for an example
//of how to declare a new method.
class Tamgua_mapiu;
//This typedef defines a type "a_mapiuMethod", which expose the typical parameters of a new Tamgu method implementation
typedef Tamgu* (Tamgua_mapiu::*a_mapiuMethod)(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
//---------------------------------------------------------------------------------------------------------------------
class Tamgua_mapiu : public TamguContainer {
    public:
    //We export the methods that will be exposed for our new object
    //this is a static object, which is common to everyone
    //We associate the method pointers with their names in the linkedmethods map
    static Exchanging basebin_hash<a_mapiuMethod> methods;
    static Exchanging hmap<string, string> infomethods;
    static Exchanging  basebin_hash<unsigned long> exported;

    static Exchanging short idtype;

    //---------------------------------------------------------------------------------------------------------------------
    //This SECTION is for your specific implementation...
    //Your personal variables here...
    atomic_map<long, atomic_wstring> values;
    bool isconst;

    //---------------------------------------------------------------------------------------------------------------------
    Tamgua_mapiu(TamguGlobal* g, Tamgu* parent = NULL) : TamguContainer(g, parent) {
     investigate |= is_string;
        //Do not forget your variable initialisation
        isconst = false;
    }

    Tamgua_mapiu() {
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
        return Tamgua_mapiu::idtype;
    }

    string Typename() {
        return "a_mapiu";
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
            Tamgua_mapiu * m = new Tamgua_mapiu;
            m->values = values;
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
        Tamgua_mapiu* m = new Tamgua_mapiu;
        long k;
        wstring vl;
        if (a->isContainer()) {
            TamguIteration* it = a->Newiteration(false);
            for (it->Begin(); it->End() == aFALSE; it->Next()) {
                k = it->Keyinteger();
                vl = it->Valueustring();
                m->values.set(k, vl);
            }
            it->Release();
            return m;
        }
        
        wstring val = a->UString();
        TamguIteration* itr = Newiteration(false);
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            k = itr->Keyinteger();
            m->values.set(k,val);
        }
        itr->Release();
        return m;
    }

    Tamgu* Newinstance(short idthread, Tamgu* f = NULL) {
        return new Tamgua_mapiu();
    }

    Exporting TamguIteration* Newiteration(bool direction);

    bool duplicateForCall() {
        return isconst;
    }

    static void AddMethod(TamguGlobal* global, string name, a_mapiuMethod func, unsigned long arity, string infos);
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

    
    Tamgu* MethodInvert(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        if (contextualpattern == this || !contextualpattern->isMapContainer() || !contextualpattern->isAffectation())
            contextualpattern = Newinstance(idthread);
        else
            contextualpattern->Clear();

        Tamgu* a;
        TamguIteration* it = Newiteration(false);
        for (it->Begin(); it->End() == aFALSE; it->Next())  {
            a = globalTamgu->Provideint(it->Keyinteger());
            contextualpattern->Push(it->Valueustring(), a);
            a->Release();
        }
        it->Release();
        return contextualpattern;
    }

    Exporting Tamgu* MethodFind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);


    Tamgu* MethodClear(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Clear();
        return aTRUE;
    }



    Tamgu* MethodItems(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamgua_mapiu* item;
        Tamgu* vect = Selectavector(contextualpattern);
        long k;
        TamguIteration* it = Newiteration(false);
        wstring vl;
        for (it->Begin(); it->End() == aFALSE; it->Next())  {
            item = new Tamgua_mapiu;
            k = it->Keyinteger();
            vl = it->Valueustring();
            item->values.set(k, vl);
            vect->Push(item);
        }
        it->Release();
        return vect;
    }

    
    Tamgu* MethodCheck(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamguivector* iv = globalTamgu->Provideivector();
        values.checkconsistence(iv->values);
        return iv;
    }
    
    Tamgu* MethodCompact(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        values.compact();
        return aTRUE;
    }
    
    Tamgu* MethodMerge(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamgu* v = callfunc->Evaluate(0, contextualpattern, idthread);
        Merging(v);
        return this;
    }

    Tamgu* MethodKeys(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamguivector* vstr = (Tamguivector*)Selectaivector(contextualpattern);
        TamguIteration* it = Newiteration(false);
        for (it->Begin(); it->End() == aFALSE; it->Next())
            vstr->values.push_back(it->Keyinteger());
        it->Release();
        return vstr;
    }

    Tamgu* MethodValues(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamguuvector * vstr = (Tamguuvector*)Selectauvector(contextualpattern);
        TamguIteration* it = Newiteration(false);
        for (it->Begin(); it->End() == aFALSE; it->Next())
            vstr->values.push_back(it->Valueustring());
        it->Release();
        return vstr;
    }

    Tamgu* MethodTest(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        long  v = callfunc->Evaluate(0, contextualpattern, idthread)->Integer();
        if (!values.check(v))
            return aFALSE;
        return aTRUE;
    }

    Tamgu* MethodPop(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        Tamgu* pos = callfunc->Evaluate(0, contextualpattern, idthread);
        return Pop(pos);
    }

    Tamgu* MethodJoin(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        //The separator between keys
        string keysep = callfunc->Evaluate(0, contextualpattern, idthread)->String();
        //The separator between values
        string sep = callfunc->Evaluate(1, contextualpattern, idthread)->String();
        bool beg = true;
        stringstream res;
        TamguIteration* it = Newiteration(false);
        for (it->Begin(); it->End() == aFALSE; it->Next()) {
            if (beg == false)
                res << sep;
            beg = false;
            res << it->Keystring() << keysep << it->Valuestring();
        }
        it->Release();
        return globalTamgu->Providestring(res.str());
    }


    //------------------------------------------------------------------------------------------
    Exporting Tamgu* Push(Tamgu* k, Tamgu* v);
    Exporting Tamgu* Pop(Tamgu* kkey);

    Tamgu* Push(long k, Tamgu* a) {
        wstring vl = a->UString();
        values.set(k, vl);
        return this;
    }

    Tamgu* Combine(Tamgu* ke) {
        //Three cases:
        if (!ke->isContainer())
            return this;
        Locking* lock = _getlock((TamguObject*)ke);


        TamguIteration* itr = ke->Newiteration(false);
        long n;
        atomic_wstring vl;
        wstring v;
        for (itr->Begin(); itr->End() == aFALSE; itr->Next()) {
            n = itr->Keyinteger();
            if (!values.check(n)) {
                v = itr->Valueustring();
                values.set(n, v);
            }
            else {
                values.get(n, vl);
                v = vl.value() + itr->Valueustring();
                values.set(n , v);
            }
        }
        itr->Release();
        _cleanlock(lock);
        return this;
    }

    Tamgu* Merging(Tamgu* ke) {
        //Three cases:
        if (!ke->isContainer())
            return this;
        Locking* lock = _getlock((TamguObject*)ke);


        long n;
        wstring vl;
        TamguIteration* itr = ke->Newiteration(false);
        for (itr->Begin(); itr->End() == aFALSE; itr->Next()) {
            n = itr->Keyinteger();
            if (!values.check(n)) {
                vl = itr->Valueustring();
                values.set(n,  vl);
            }
        }
        _cleanlock(lock);
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
    Exporting wstring UString();
    Exporting string JSonString();

    Tamgu* Value(long n) {
        if (!values.check(n))
            return aNOELEMENT;
        atomic_wstring w;
        values.get(n, w);
        return globalTamgu->Provideustring(w.value());
    }

    Tamgu* Value(Tamgu* a) {
        long v =  a->Integer();
        if (!values.check(v))
            return aNOELEMENT;
        atomic_wstring w;
        values.get(v, w);
        return globalTamgu->Provideustring(w.value());
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
class TamguIterationa_mapiu : public TamguIteration {
    public:

    long i;
    long first;
    atomic_wstring second;
    short indexarray;
    bool end;
    atomic_element<long, atomic_wstring>* e;
    uchar idx;
    BULONG filter;
    
    Tamgua_mapiu* mm;
    
    TamguIterationa_mapiu(Tamgua_mapiu* m, bool d, TamguGlobal* g = NULL) : TamguIteration(d, g) {
        mm = m;
        end = false;
        i = 0;
        idx = 255;
        indexarray = 0;
        filter = 0;
        first = 0;
    }

    
    void Setvalue(Tamgu* recipient, short idthread) {
        recipient->storevalue(first);
    }

    Tamgu* Key() {
        return globalTamgu->Provideint(first);
    }

    Tamgu* Value() {
        return new Tamgua_ustring(second);
    }

    

    string Keystring() {
        return convertfromnumber(first);
    }

    wstring Keyustring() {
        return wconvertfromnumber(first);
    }

    BLONG Keylong() {
        return first;
    }

    long Keyinteger() {
        return first;
    }

    
    short Keyshort() {
        return first;
    }

    double Keyfloat() {
        return first;
    }



    wstring Valueustring() {
        return second.value();
        
    }

    string Valuestring() {
        string s;
        wstring w = second.value();
        s_unicode_to_utf8(s, w);
        return s;        
    }


    
    void Next() {
        end = mm->values.iterate(&e, first, second, indexarray, idx, filter);
    }

    Tamgu* End() {
        if (end)
            return aTRUE;
        return aFALSE;
    }

    Tamgu* Begin() {
        end = false;
        e = NULL;
        idx = 255;
        indexarray = 0;
        filter = 0;
        second.clear();
        mm->values.Begin(&e);
        end = mm->values.iterate(&e, first, second, indexarray, idx, filter);
        return aTRUE;
    }
    
    void find(long v) {
        end = false;
        e = NULL;
        idx = 255;
        indexarray = 0;
        first = 0;
        second.clear();

        mm->values.find(v, &e, indexarray, idx, filter, end);
    }

    bool compare(TamguIterationa_mapiu& it) {
        if (it.i == i && it.idx == idx)
            return true;
        return false;
    }
};


//-------------------------------------------------------------------------------------------

#endif
