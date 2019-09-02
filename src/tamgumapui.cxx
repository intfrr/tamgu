/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamgumapui.cxx
 Date       : 2017/09/01
 Purpose    : map implementation
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#include "tamgu.h"
#include "tamguuvector.h"
#include "tamguustring.h"
//#prime
#include "tamgumapui.h"
#include "tamgulist.h"
#include "tamguivector.h"


//We need to declare once again our local definitions.
Exporting basebin_hash<mapuiMethod>  Tamgumapui::methods;
Exporting hmap<string, string> Tamgumapui::infomethods;
Exporting bin_hash<unsigned long> Tamgumapui::exported;

Exporting short Tamgumapui::idtype = 0;

//-------------------------------------------------------------------------
//MethodInitialization will add the right references to "name", which is always a new method associated to the object we are creating
void Tamgumapui::AddMethod(TamguGlobal* global, string name,mapuiMethod func, unsigned long arity, string infos) {
    short idname = global->Getid(name);
    methods[idname] = func;
    infomethods[name] = infos;
    exported[idname] = arity;
}



    void Tamgumapui::Setidtype(TamguGlobal* global) {
        Tamgumapui::idtype = global->Getid("mapui");
    }

   bool Tamgumapui::InitialisationModule(TamguGlobal* global, string version) {
    methods.clear();
    infomethods.clear();
    exported.clear();

    
    Tamgumapui::idtype = global->Getid("mapui");

    Tamgumapui::AddMethod(global, "clear", &Tamgumapui::MethodClear, P_NONE, "clear(): clear the container.");
    
    Tamgumapui::AddMethod(global, "invert", &Tamgumapui::MethodInvert, P_NONE, "invert(): return a map with key/value inverted.");
    Tamgumapui::AddMethod(global, "find", &Tamgumapui::MethodFind, P_ONE, "find(value): test if a value belongs to the map and return 'true' or the corresponding keys.");


    Tamgumapui::AddMethod(global, "items", &Tamgumapui::MethodItems, P_NONE, "items(): Return a vector of {key:value} pairs.");


    Tamgumapui::AddMethod(global, "join", &Tamgumapui::MethodJoin, P_TWO, "join(string sepkey,string sepvalue): Produce a string representation for the container.");
    Tamgumapui::AddMethod(global, "test", &Tamgumapui::MethodTest, P_ONE, "test(key): Test if key belongs to the map container.");
    Tamgumapui::AddMethod(global, "keys", &Tamgumapui::MethodKeys, P_NONE, "keys(): Return the map container keys as a vector.");
    Tamgumapui::AddMethod(global, "values", &Tamgumapui::MethodValues, P_NONE, "values(): Return the map container values as a vector.");
    Tamgumapui::AddMethod(global, "sum", &Tamgumapui::MethodSum, P_NONE, "sum(): return the sum of elements.");
    Tamgumapui::AddMethod(global, "product", &Tamgumapui::MethodProduct, P_NONE, "product(): return the product of elements.");
    Tamgumapui::AddMethod(global, "pop", &Tamgumapui::MethodPop, P_ONE, "pop(key): Erase an element from the map");
    Tamgumapui::AddMethod(global, "merge", &Tamgumapui::MethodMerge, P_ONE, "merge(v): Merge v into the vector.");

    global->newInstance[Tamgumapui::idtype] = new Tamgumapui(global);
    #ifdef OLDBACKCOMP
    global->newInstance[global->Getid("uimap")] = new Tamgumapui(global);

    global->RecordMethods(global->Getid("uimap"), Tamgumapui::exported);
    #endif
    global->RecordMethods(Tamgumapui::idtype, Tamgumapui::exported);

    return true;
}


Exporting TamguIteration* Tamgumapui::Newiteration(bool direction) {
    return new TamguIterationmapui(this, direction);
}


Exporting Tamgu* Tamgumapui::in(Tamgu* context, Tamgu* a, short idthread) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...
    
    wstring val = a->UString();

     if (context->isVectorContainer()) {
        Tamguuvector* v = (Tamguuvector*)Selectauvector(context);
        Doublelocking _lock(this, v);
        if (values.find(val)!=values.end())
            v->values.push_back(val);

        return v;
    }

   if (context->isString()) {
        Locking _lock(this);
        if (values.find(val)!=values.end())
            return globalTamgu->Provideustring(val);
        return aNOELEMENT;
    }
    
    Locking _lock(this);
    if (values.find(val)!=values.end())
        return aTRUE;

    return aFALSE;

}

Exporting Tamgu* Tamgumapui::MethodFind(Tamgu* context, short idthread, TamguCall* callfunc) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...

    
    long val = callfunc->Evaluate(0, context, idthread)->Integer();
    
    if (context->isBoolean()) {
        Locking _lock(this);
        for (auto& it : values) {
            if (it.second == val)
                return aTRUE;
        }
        return aFALSE;
    }
    if (context->isVectorContainer()) {
        Tamguuvector* v = (Tamguuvector*)Selectauvector(context);
        Doublelocking _lock(this, v);
        for (auto& it : values) {
            if (it.second == val)
                v->values.push_back(it.first);
        }
        return v;
    }
    
    Locking _lock(this);
    for (auto& it : values) {
        if (it.second == val)
            return globalTamgu->Provideustring(it.first);
    }
    return aNULL;
}



Exporting Tamgu* Tamgumapui::Push(Tamgu* k, Tamgu* v) {
    Locking _lock(this);
    wstring s = k->UString();
    values[s] = v->Integer();
    return aTRUE;
}

Exporting Tamgu* Tamgumapui::Pop(Tamgu* kkey) {
    wstring k = kkey->UString();
    Locking _lock(this);
    if (values.find(k) != values.end()) {
        values.erase(k);
        return aTRUE;
    }
    return aFALSE;
}

Exporting void Tamgumapui::Clear() {
    Locking _lock(this);
    values.clear();
}



Exporting string Tamgumapui::String() {
    Locking _lock(this);
    stringstream res;

    res << "{";
    bool beg = true;
    string sx;
    wstring w;
    for (auto& it : values) {
        if (beg == false)
            res << ",";
        beg = false;
        w = it.first;
        sc_unicode_to_utf8(sx, w);
        stringing(res, sx);
        res << ":" << it.second;
    }
    res << "}";
    return res.str();
}

Exporting string Tamgumapui::JSonString() {
    Locking _lock(this);
    stringstream res;

    res << "{";
    bool beg = true;
    string sx;
    wstring w;
    for (auto& it : values) {
        if (beg == false)
            res << ",";
        beg = false;
        w = it.first;
        sc_unicode_to_utf8(sx, w);
        jstringing(res, sx);
        res << ":" << it.second;
    }
    res << "}";
    return res.str();
}


Exporting long Tamgumapui::Integer() {
    Locking _lock(this);
    return values.size();
}

Exporting double Tamgumapui::Float() {
    Locking _lock(this);
    return values.size();
}

Exporting BLONG Tamgumapui::Long() {
    Locking _lock(this);
    return values.size();
}

Exporting bool Tamgumapui::Boolean() {
    Locking _lock(this);
    if (values.size() == 0)
        return false;
    return true;
}


//Basic operations
Exporting long Tamgumapui::Size() {
    Locking _lock(this);
    return values.size();
}

#ifndef swprintf_s
#define swprintf_s(a,b,c,d) swprintf(a,b,c,d)
    #endif


Exporting Tamgu*  Tamgumapui::Put(Tamgu* idx, Tamgu* ke, short idthread) {
    if (!idx->isIndex()) {
        if (ke == this)
            return aTRUE;

        if (ke->isNULL()) {
            Clear();
            return aTRUE;
        }
        
        if (ke->Type() == Tamgumapui::idtype) {
            Doublelocking _lock(this, ke);
            values = ((Tamgumapui*)ke)->values;
            return aTRUE;
        }

        if (ke->isMapContainer()) {
            Doublelocking _lock(this, ke);
            values.clear();
            TamguIteration* itr = ke->Newiteration(false);
            for (itr->Begin(); itr->End() == aFALSE; itr->Next())
                values[itr->Keyustring()] = itr->Valueinteger();
            itr->Release();
            return aTRUE;
        }
        wchar_t buff[20];
        if (ke->isVectorContainer()) {
            Doublelocking _lock(this, ke);
            values.clear();
            long nb = 0;
            for (long it = 0; it < ke->Size(); ++it) {
                swprintf_s(buff, 20, L"%d", nb);
                values[buff] = ke->getinteger(it);
                nb++;
            }
            return aTRUE;
        }
        if (ke->Type() == a_list) {
            Doublelocking _lock(this, ke);
            Tamgulist* kvect = (Tamgulist*)ke;
            values.clear();
            long nb = 0;

            for (auto& it : kvect->values) {
                swprintf_s(buff, 20, L"%d", nb);
                values[buff] = it->Integer();
                nb++;
            }
            return aTRUE;
        }
        ke = ke->Map(idthread);
        if (!ke->isMapContainer())
            return globalTamgu->Returnerror("Wrong map initialization", idthread);
        Locking* _lock = _getlock(this);
        values.clear();
        if (ke->Type() == Tamgumapui::idtype)
            values = ((Tamgumapui*)ke)->values;
        else {
            TamguIteration* itr = ke->Newiteration(false);
            for (itr->Begin(); itr->End() != aTRUE; itr->Next())
                values[itr->Keyustring()] = itr->Valueinteger();
            itr->Release();
        }
        ke->Release();
        _cleanlock(_lock);
        return aTRUE;
    }
    Locking* _lock = _getlock(this);
    values[idx->UString()] = ke->Integer();
    _cleanlock(_lock);
    return aTRUE;
}


Exporting Tamgu* Tamgumapui::Eval(Tamgu* contextualpattern, Tamgu* idx, short idthread) {

    Locking _lock(this);

    if (!idx->isIndex()) {
        //particular case, the contextualpattern is a vector, which means that we expect a set of keys
        //as a result
                if (contextualpattern->isMapContainer())
            return this;
        
       //particular case, the contextualpattern is a vector, which means that we expect a set of keys
        //as a result
        if (contextualpattern->isVectorContainer() || contextualpattern->Type() == a_list) {
            Tamgu* vect = contextualpattern->Newinstance(idthread);
            hmap<wstring,long>::iterator it;
            for (it = values.begin(); it != values.end(); it++)
                vect->Push(globalTamgu->Provideustring(it->first));
            return vect;
        }

        if (contextualpattern->isNumber()) {
            long v = Size();
            return globalTamgu->Provideint(v);
        }

        return this;
    }

    Tamgu* key;
    if (idx->isInterval()) {
        Tamgumapui* kmap = new Tamgumapui;
        key = ((TamguIndex*)idx)->left->Eval(aNULL, aNULL, idthread);
        Tamgu* keyright = ((TamguIndex*)idx)->right->Eval(aNULL, aNULL, idthread);
        wstring vleft = key->UString();
        wstring vright = keyright->UString();
        hmap<wstring,long>::iterator it = values.find(vleft);
        if (it == values.end() && key != aNULL) {
            key->Release();
            return kmap;
        }
        if (key == aNULL)
            it = values.begin();
        key->Release();
        hmap<wstring, long>::iterator itr= values.end();
        if (keyright != aNULL) {
            itr = values.find(vright);
            if (itr == values.end()) {
                keyright->Release();
                return kmap;
            }
        }
        keyright->Release();
        for (; it != values.end(); it++) {
            kmap->values[it->first] = it->second;
            if (it == itr)
                return kmap;
        }

        if (itr != values.end())
            kmap->values.clear();
        return kmap;

    }

    key = ((TamguIndex*)idx)->left->Eval(aNULL, aNULL, idthread);
    
    if (key == aNULL) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong index", idthread);
        return aNOELEMENT;
    }

    wstring skey = key->UString();
    key->Release();

    Tamgu* kval = Value(skey);
    if (kval == aNOELEMENT) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong index", idthread);
        return aNOELEMENT;
    }
    return kval;
}

Exporting Tamgu* Tamgumapui::same(Tamgu* a) {

    if (a->Type() != idtype)
        return Mapcompare(this, a, NULL);

    Tamgumapui* m = (Tamgumapui*)a;

    Doublelocking _lock(this, m);
    if (m->values.size() != values.size())
        return aFALSE;
    hmap<wstring,long>::iterator it = m->values.begin();
    while (it != m->values.end()) {
        if (values.find(it->first) == values.end())
            return aFALSE;
        if (it->second != values[it->first])
            return aFALSE;
        it++;
    }
    return aTRUE;
}

Exporting Tamgu* Tamgumapui::xorset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);
    
    Tamgumapui* res;
    
    
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        hmap<wstring, long> keys;

        for (auto& it : values)
            keys[it.first] = it.second;
            
        wstring v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Keyustring();
            if (keys.find(v) == keys.end())
                keys[v]=itr->Valueinteger();
            else {
                if (values[v]==itr->Valueinteger())
                    keys.erase(v);
            }
        }
        itr->Release();

        for (auto& a : keys)
            res->values[a.first]= a.second;

        return res;


    }
    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : values)
        it.second = it.second ^ v;
    return res;
}

Exporting Tamgu* Tamgumapui::orset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui* res;
    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);
    if (b->isMapContainer()) {
        res->Merging(b);
        return res;
    }
    long v = b->Integer();
    for (auto& it : res->values)
        it.second = it.second | v;
    return res;
}

Exporting Tamgu* Tamgumapui::andset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui* res;
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);
        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            if (values.find(itr->Keyustring()) != values.end() && values[itr->Keyustring()]==v)
                res->values[itr->Keyustring()] = v;
        }
        itr->Release();
        return res;
    }
    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);
    long v = b->Integer();
    for (auto& it : res->values)
        it.second = it.second & v;
    return res;
}

Exporting Tamgu* Tamgumapui::plus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui* res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second + v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : res->values)
        it.second += v;
    return res;
}

Exporting Tamgu* Tamgumapui::minus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second + v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : res->values)
        it.second -= v;
    return res;
}

Exporting Tamgu* Tamgumapui::multiply(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second * v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : res->values)
        it.second *= v;
    return res;

}

Exporting Tamgu* Tamgumapui::divide(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            if (v == 0) {
                res->Release();
                return globalTamgu->Returnerror("Error: Divided by 0");
            }

            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second * v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    if (v == 0) {
        res->Release();
        return globalTamgu->Returnerror("Error: Divided by 0");
    }
    for (auto& it : res->values)
        it.second /= v;
    return res;

}

Exporting Tamgu* Tamgumapui::mod(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            if (v == 0) {
                res->Release();
                return globalTamgu->Returnerror("Error: Divided by 0");
            }

            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second  % v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    if (v == 0) {
        res->Release();
        return globalTamgu->Returnerror("Error: Divided by 0");
    }
    for (auto& it : res->values)
        it.second = it.second % v;
    return res;
}

Exporting Tamgu* Tamgumapui::shiftright(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second >> v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : res->values)
        it.second = it.second >> v;
    return res;
}

Exporting Tamgu* Tamgumapui::shiftleft(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        long v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valueinteger();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = it->second << v;
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    long v = b->Integer();
    for (auto& it : res->values)
        it.second = it.second << v;
    return res;
}

Exporting Tamgu* Tamgumapui::power(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    Tamgumapui * res;
    if (b->isMapContainer()) {
        hmap<wstring,long>::iterator it;

        TamguIteration* itr = b->Newiteration(false);

        res = new Tamgumapui;
        double v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valuefloat();
            it = values.find(itr->Keyustring());
            if (it != values.end()) {
                res->values[it->first] = pow(it->second, v);
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamgumapui*)Atom(true);

    double v = b->Float();
    for (auto& it : res->values)
        it.second = pow(it.second, v);
    return res;
}

Exporting Tamgu* Tamgumapui::Loopin(TamguInstruction* ins, Tamgu* context, short idthread) {
    Locking _lock(this);
    Tamgu* var = ins->instructions.vecteur[0]->Instruction(0);
    var = var->Eval(context, aNULL, idthread);

    
    hmap<wstring, long>::iterator it;
    
    Tamgu* a;
    vector<wstring> keys;

    for (it=values.begin(); it != values.end(); it++)
        keys.push_back(it->first);

    for (long i = 0; i < keys.size(); i++) {

        var->storevalue(keys[i]);

        a = ins->instructions.vecteur[1]->Eval(context, aNULL, idthread);

        //Continue does not trigger needInvestigate
        if (a->needInvestigate()) {
            if (a == aBREAK)
                break;
            return a;
        }

        a->Release();
    }

    return this;

}
