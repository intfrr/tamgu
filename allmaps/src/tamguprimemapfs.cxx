/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamguprimemapfs.cxx
 Date       : 2017/09/01
 Purpose    : map implementation
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#include "tamgu.h"
#include "tamgufvector.h"
#include "tamgufloat.h"
//#prime
#include "tamguprimemapfs.h"
#include "tamgulist.h"
#include "tamguivector.h"


//We need to declare once again our local definitions.
Exporting basebin_hash<primemapfsMethod>  Tamguprimemapfs::methods;
Exporting hmap<string, string> Tamguprimemapfs::infomethods;
Exporting bin_hash<unsigned long> Tamguprimemapfs::exported;

Exporting short Tamguprimemapfs::idtype = 0;

//-------------------------------------------------------------------------
//MethodInitialization will add the right references to "name", which is always a new method associated to the object we are creating
void Tamguprimemapfs::AddMethod(TamguGlobal* global, string name,primemapfsMethod func, unsigned long arity, string infos) {
    short idname = global->Getid(name);
    methods[idname] = func;
    infomethods[name] = infos;
    exported[idname] = arity;
}

bool Tamguprimemapfs::InitialisationModule(TamguGlobal* global, string version) {
    methods.clear();
    infomethods.clear();
    exported.clear();

    
    Tamguprimemapfs::idtype = global->Getid("primemapfs");

    Tamguprimemapfs::AddMethod(global, "clear", &Tamguprimemapfs::MethodClear, P_NONE, "clear(): clear the container.");
    
    Tamguprimemapfs::AddMethod(global, "invert", &Tamguprimemapfs::MethodInvert, P_NONE, "invert(): return a map with key/value inverted.");
    Tamguprimemapfs::AddMethod(global, "find", &Tamguprimemapfs::MethodFind, P_ONE, "find(value): test if a value belongs to the map and return 'true' or the corresponding keys.");


    Tamguprimemapfs::AddMethod(global, "items", &Tamguprimemapfs::MethodItems, P_NONE, "items(): Return a vector of {key:value} pairs.");


    Tamguprimemapfs::AddMethod(global, "join", &Tamguprimemapfs::MethodJoin, P_TWO, "join(string sepkey,string sepvalue): Produce a string representation for the container.");
    Tamguprimemapfs::AddMethod(global, "test", &Tamguprimemapfs::MethodTest, P_ONE, "test(key): Test if key belongs to the map container.");
    Tamguprimemapfs::AddMethod(global, "keys", &Tamguprimemapfs::MethodKeys, P_NONE, "keys(): Return the map container keys as a vector.");
    Tamguprimemapfs::AddMethod(global, "values", &Tamguprimemapfs::MethodValues, P_NONE, "values(): Return the map container values as a vector.");
    Tamguprimemapfs::AddMethod(global, "pop", &Tamguprimemapfs::MethodPop, P_ONE, "pop(key): Erase an element from the map");
    Tamguprimemapfs::AddMethod(global, "merge", &Tamguprimemapfs::MethodMerge, P_ONE, "merge(v): Merge v into the vector.");

    global->newInstance[Tamguprimemapfs::idtype] = new Tamguprimemapfs(global);
    #ifdef OLDBACKCOMP
    global->newInstance[global->Getid("fsprimemap")] = new Tamguprimemapfs(global);

    global->RecordMethods(global->Getid("fsprimemap"), Tamguprimemapfs::exported);
    #endif
    global->RecordMethods(Tamguprimemapfs::idtype, Tamguprimemapfs::exported);

    return true;
}


Exporting TamguIteration* Tamguprimemapfs::Newiteration(bool direction) {
    return new TamguIterationprimemapfs(this, direction);
}


Exporting Tamgu* Tamguprimemapfs::in(Tamgu* context, Tamgu* a, short idthread) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...
    
    double val = a->Float();

     if (context->isVectorContainer()) {
        Tamgufvector* v = (Tamgufvector*)Selectafvector(context);
        Doublelocking _lock(this, v);
        if (values.find(val)!=values.end())
            v->values.push_back(val);

        return v;
    }

   if (context->isNumber()) {
        Locking _lock(this);
        if (values.find(val)!=values.end())
            return globalTamgu->Providefloat(val);
        return aNOELEMENT;
    }
    
    Locking _lock(this);
    if (values.find(val)!=values.end())
        return aTRUE;

    return aFALSE;

}

Exporting Tamgu* Tamguprimemapfs::MethodFind(Tamgu* context, short idthread, TamguCall* callfunc) {
    //Three cases along the container type...
    //It is a Boolean, we expect false or true
    //It is an integer, we expect a position in v
    //It is a container, we are looking for all positions...

    prime_hash<double, string>::iterator it;
    
    string val = callfunc->Evaluate(0, context, idthread)->String();
    
    if (context->isBoolean()) {
        Locking _lock(this);
        for (it = values.begin(); it != values.end(); it++) {
            if (it->second == val)
                return aTRUE;
        }
        return aFALSE;
    }

    if (context->isVectorContainer()) {
        Tamgufvector* v = (Tamgufvector*)Selectafvector(context);
        Doublelocking _lock(this, v);
        for (it = values.begin(); it != values.end(); it++) {
            if (it->second == val)
                v->values.push_back(it->first);
        }
        return v;
    }
    
    Locking _lock(this);
    for (it = values.begin(); it != values.end(); it++) {
        if (it->second == val)
            return globalTamgu->Providefloat(it->first);
    }

    return aNULL;

}



Exporting Tamgu* Tamguprimemapfs::Push(Tamgu* k, Tamgu* v) {
    Locking _lock(this);
    double s = k->Float();
    values[s] = v->String();
    return aTRUE;
}

Exporting Tamgu* Tamguprimemapfs::Pop(Tamgu* kkey) {
    double k = kkey->Float();
    Locking _lock(this);
    if (values.find(k) != values.end()) {
        values.erase(k);
        return aTRUE;
    }
    return aFALSE;
}

Exporting void Tamguprimemapfs::Clear() {
    Locking _lock(this);
    values.clear();
}



Exporting string Tamguprimemapfs::String() {
    Locking _lock(this);
    stringstream res;
    prime_hash<double,string>::iterator it;
    res << "{";
    bool beg = true;
    for (it = values.begin(); it != values.end(); it++) {
        if (beg == false)
            res << ",";
        beg = false;
        res << it->first << ":";
        stringing(res, it->second);
    }
    res << "}";
    return res.str();
}

Exporting string Tamguprimemapfs::JSonString() {
    Locking _lock(this);
    stringstream res;
    prime_hash<double,string>::iterator it;
    res << "{";
    bool beg = true;
    for (it = values.begin(); it != values.end(); it++) {
        if (beg == false)
            res << ",";
        beg = false;
        res << '"' << it->first << '"' << ":";
        jstringing(res, it->second);
    }
    res << "}";
    return res.str();
}


Exporting long Tamguprimemapfs::Integer() {
    Locking _lock(this);
    return values.size();
}

Exporting double Tamguprimemapfs::Float() {
    Locking _lock(this);
    return values.size();
}

Exporting BLONG Tamguprimemapfs::Long() {
    Locking _lock(this);
    return values.size();
}

Exporting bool Tamguprimemapfs::Boolean() {
    Locking _lock(this);
    if (values.size() == 0)
        return false;
    return true;
}


//Basic operations
Exporting long Tamguprimemapfs::Size() {
    Locking _lock(this);
    return values.size();
}


Exporting Tamgu*  Tamguprimemapfs::Put(Tamgu* idx, Tamgu* ke, short idthread) {

    
    if (!idx->isIndex()) {
        if (ke == this)
            return aTRUE;

        if (ke->isNULL()) {
            Clear();
            return aTRUE;
        }

        
        if (ke->Type() == Tamguprimemapfs::idtype) {
            Doublelocking _lock(this, ke);
            values = ((Tamguprimemapfs*)ke)->values;
            return aTRUE;
        }

        if (ke->isMapContainer()) {
            Doublelocking _lock(this, ke);
            values.clear();
            TamguIteration* itr = ke->Newiteration(false);
            for (itr->Begin(); itr->End() == aFALSE; itr->Next())
                values[itr->Keyfloat()] = itr->Valuestring();
            itr->Release();
            return aTRUE;
        }
        
        if (ke->isVectorContainer()) {
            Doublelocking _lock(this, ke);
            values.clear();
            long nb = 0;
            for (long it = 0; it < ke->Size(); ++it) {
                values[nb] = ke->getstring(it);
                nb++;
            }
            return aTRUE;
        }

        if (ke->Type() == a_list) {
            Doublelocking _lock(this, ke);
            Tamgulist* kvect = (Tamgulist*)ke;
            values.clear();
            long nb = 0;
            list<Tamgu*>::iterator it;
            for (it = kvect->values.begin(); it != kvect->values.end(); it++) {
                values[nb] = (*it)->String();
                nb++;
            }
            return aTRUE;
        }

        ke = ke->Map(idthread);
        if (!ke->isMapContainer())
            return globalTamgu->Returnerror("Wrong map initialization", idthread);

        Locking* _lock = _getlock(this);
        values.clear();
        if (ke->Type() == Tamguprimemapfs::idtype) {
            Tamguprimemapfs* kmap = (Tamguprimemapfs*)ke;
            //We copy all values from ke to this
            prime_hash<double,string>::iterator it;
            for (it = kmap->values.begin(); it != kmap->values.end(); it++)
                values[it->first] = it->second;
        }
        else {
            TamguIteration* itr = ke->Newiteration(false);
            for (itr->Begin(); itr->End() != aTRUE; itr->Next())
                values[itr->Keyfloat()] = itr->Valuestring();
            itr->Release();
        }
        ke->Release();
        _cleanlock(_lock);
        return aTRUE;
    }

    Locking* _lock = _getlock(this);
    values[idx->Float()] = ke->String();
    _cleanlock(_lock);
    return aTRUE;
}


Exporting Tamgu* Tamguprimemapfs::Eval(Tamgu* contextualpattern, Tamgu* idx, short idthread) {

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
            prime_hash<double,string>::iterator it;
            for (it = values.begin(); it != values.end(); it++)
                vect->Push(globalTamgu->Providefloat(it->first));
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
        Tamguprimemapfs* kmap = new Tamguprimemapfs;
        key = ((TamguIndex*)idx)->left->Eval(aNULL, aNULL, idthread);
        Tamgu* keyright = ((TamguIndex*)idx)->right->Eval(aNULL, aNULL, idthread);
        double vleft = key->Float();
        double vright = keyright->Float();
        prime_hash<double,string>::iterator it = values.find(vleft);
        if (it == values.end() && key != aNULL) {
            key->Release();
            return kmap;
        }
        if (key == aNULL)
            it = values.begin();
        key->Release();
        prime_hash<double, string>::iterator itr= values.end();
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

    double skey = key->Float();
    key->Release();

    Tamgu* kval = Value(skey);
    if (kval == aNOELEMENT) {
        if (globalTamgu->erroronkey)
            return globalTamgu->Returnerror("Wrong index", idthread);
        return aNOELEMENT;
    }
    return kval;
}

Exporting Tamgu* Tamguprimemapfs::same(Tamgu* a) {

    if (a->Type() != idtype)
        return Mapcompare(this, a, NULL);

    Tamguprimemapfs* m = (Tamguprimemapfs*)a;

    Doublelocking _lock(this, m);
    if (m->values.size() != values.size())
        return aFALSE;
    prime_hash<double,string>::iterator it = m->values.begin();
    while (it != m->values.end()) {
        if (values.find(it->first) == values.end())
            return aFALSE;
        if (it->second != values[it->first])
            return aFALSE;
        it++;
    }
    return aTRUE;
}

Exporting Tamgu* Tamguprimemapfs::xorset(Tamgu* b, bool itself) {

    Doublelocking _lock(this, b);


    Tamguprimemapfs* res;
    
    
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = new Tamguprimemapfs;
        hmap<double, string> keys;

        prime_hash<double, string>::iterator it;
        for (it = values.begin(); it != values.end(); it++)
            keys[it->first] = it->second;
            
        double v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Keyfloat();
            if (keys.find(v) == keys.end())
                keys[v]=itr->Valuestring();
            else {
                if (values[v]==itr->Valuestring())
                    keys.erase(v);
            }
        }
        itr->Release();

        for (auto& a : keys)
            res->values[a.first]= a.second;

        return res;


    }

    prime_hash<double, string>::iterator it;
    if (itself)
        res = this;
    else
        res = (Tamguprimemapfs*)Atom(true);

    string v = b->String();
    for (it = values.begin(); it != values.end(); it++)
        it->second = StringXor(it->second,v);
    return res;
}

Exporting Tamgu* Tamguprimemapfs::orset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    prime_hash<double,string>::iterator it;
    Tamguprimemapfs* res;
    if (itself)
        res = this;
    else
        res = (Tamguprimemapfs*)Atom(true);

    if (b->isMapContainer()) {
        res->Merging(b);
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamguprimemapfs*)Atom(true);

    string v = b->String();
    for (it = values.begin(); it != values.end(); it++)
        it->second = StringOr(it->second,v);
    return res;
}

Exporting Tamgu* Tamguprimemapfs::andset(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    prime_hash<double,string>::iterator it;
    Tamguprimemapfs* res;
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = new Tamguprimemapfs;
        string v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valuestring();
            if (values.find(itr->Keyfloat()) != values.end() && values[itr->Keyfloat()]==v)
                res->values[itr->Keyfloat()] = v;
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamguprimemapfs*)Atom(true);

    string v = b->String();
    for (it = values.begin(); it != values.end(); it++)
        it->second = StringAnd(it->second,v);
    return res;
}

Exporting Tamgu* Tamguprimemapfs::plus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    prime_hash<double,string>::iterator it;
    Tamguprimemapfs* res;
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = new Tamguprimemapfs;
        string v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valuestring();
            it = values.find(itr->Keyfloat());
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
        res = (Tamguprimemapfs*)Atom(true);

    string v = b->String();
    for (it = values.begin(); it != values.end(); it++)
        it->second += v;
    return res;
}

Exporting Tamgu* Tamguprimemapfs::minus(Tamgu* b, bool itself) {
    Doublelocking _lock(this, b);

    prime_hash<double,string>::iterator it;
    Tamguprimemapfs * res;
    if (b->isMapContainer()) {
        TamguIteration* itr = b->Newiteration(false);

        res = new Tamguprimemapfs;
        string v;
        for (itr->Begin(); itr->End() != aTRUE; itr->Next()) {
            v = itr->Valuestring();
            it = values.find(itr->Keyfloat());
            if (it != values.end()) {
                res->values[it->first] = StringMinus(it->second, v);
            }
        }
        itr->Release();
        return res;
    }

    if (itself)
        res = this;
    else
        res = (Tamguprimemapfs*)Atom(true);

    string v = b->String();
    for (it = values.begin(); it != values.end(); it++)
        it->second = StringMinus(it->second,v);
    return res;
}

Exporting Tamgu* Tamguprimemapfs::Loopin(TamguInstruction* ins, Tamgu* context, short idthread) {
    Locking _lock(this);
    Tamgu* var = ins->instructions.vecteur[0]->Instruction(0);
    var = var->Eval(context, aNULL, idthread);

    
    prime_hash<double, string>::iterator it;
    
    Tamgu* a;
    vector<double> keys;

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
