/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : tamgustring.h
 Date       : 2017/09/01
 Purpose    :  
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
*/

#ifndef tamgustring_h
#define tamgustring_h

#include "tamguint.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//---------------------------------------------------------------------------------------------------------------------
const long base_atomic_string_size = 7;
const long atomic_string_size = 1 << base_atomic_string_size;

class atomic_string_element {
public:
    
    char buffer[atomic_string_size+1];
    atomic_string_element* next;
    
    atomic_string_element() {
        buffer[0] = 0;
        buffer[atomic_string_size] = 0;
        next = NULL;
    }
    
    ~atomic_string_element() {
        if (next != NULL)
            delete next;
    }
    
    void reset() {
        buffer[0] = 0;
        buffer[atomic_string_size] = 0;
        if (next != NULL)
            next->reset();
    }
    
    void copy(string& v, long sz, long pos, std::recursive_mutex* lock) {
        long i;
        if ((sz-pos) < atomic_string_size) {
            for (i = 0; i < sz-pos; i++)
                buffer[i] = v[i+pos];
            buffer[sz-pos]=0;
        }
        else {
            for (i = 0; i < atomic_string_size; i++)
                buffer[i] = v[i+pos];
            buffer[atomic_string_size] = 0;
            
            if (sz > pos+atomic_string_size) {
                if (next == NULL) {
                    lock->lock();
                    if (next == NULL)
                        next = new atomic_string_element;
                    lock->unlock();
                    
                }
                next->copy(v,sz,pos+atomic_string_size, lock);
            }
        }
    }
    
    long size() {
        long sz = strlen(buffer);
        if (next != NULL)
            sz += next->size();
        return sz;
    }
    
    void copy(atomic_string_element* a, std::recursive_mutex* lock) {
        long sz = strlen(a->buffer);
        for (long i = 0; i < sz; i++)
            buffer[i] = a->buffer[i];
        buffer[sz] = 0;
        
        if (a->next) {
            if (next == NULL) {
                lock->lock();
                if (next == NULL)
                    next = new atomic_string_element;
                lock->unlock();
            }
            
            next->copy(a->next, lock);
        }
    }
    
    void concat(string& v, long nb, long i, std::recursive_mutex* lock) {
        atomic_string_element* n = this;
        while (nb) {
            if (n->next == NULL) {
                lock->lock();
                if (n->next == NULL)
                    n->next = new atomic_string_element;
                lock->unlock();
            }
            n = n->next;
            --nb;
        }
        
        long szv = v.size();
        long j = 0;
        while (i < atomic_string_size && j < szv)
            n->buffer[i++] = v[j++];
        
        if (i == atomic_string_size || j == szv)
            n->buffer[i] = 0;
        
        if (j < szv) {
            if (n->next == NULL) {
                lock->lock();
                if (n->next == NULL)
                    n->next = new atomic_string_element;
                lock->unlock();
            }
            n->next->copy(v, szv, j, lock);
        }
    }
    
    bool compare(atomic_string_element* a) {
        if (strcmp(buffer, a->buffer))
            return false;
        if (a->next) {
            if (next)
                return next->compare(a->next);
            else
                return false;
        }
        return true;
    }

};

class atomic_string {
public:
    atomic_string_element* head;
    std::recursive_mutex* lock;

    atomic_string() {
        lock = new std::recursive_mutex;
        head = new atomic_string_element;
    }


    atomic_string(string& v) {
        lock = new std::recursive_mutex;
        head = new atomic_string_element;
        head->copy(v, v.size(), 0, lock);
  }

    ~atomic_string() {
        delete lock;
        delete head;
    }
    
    void reset() {
        head->reset();
    }
    
    void clear() {
        head->reset();
    }

    void concat(string v) {
        long sz = v.size();
        if (!sz)
            return;
        
        if (isempty()) {
            head->copy(v, sz, 0, lock);
            return;
        }
        
        long size  = head->size();

        long nb = size >> base_atomic_string_size;
        long i = size - (nb << base_atomic_string_size);
        head->concat(v, nb, i, lock);
    }
    
    atomic_string& operator=(string v) {
        reset();
        long sz = v.size();
        if (sz)
            head->copy(v, sz, 0, lock);
        return *this;
    }
    
    atomic_string& operator=(atomic_string& a) {
        reset();
        if (!a.isempty())
            head->copy(a.head, lock);
        return *this;
    }
    
    char operator[](long pos) {
        long nb = pos >> base_atomic_string_size;
        long i = pos - (nb << base_atomic_string_size);
        atomic_string_element* n = head;
        while (nb > 0) {
            if (n == NULL)
                return 0;
            --nb;
            n = n->next;
        }
        
        if (n == NULL || i >= strlen(n->buffer))
            return 0;
        
        return n->buffer[i];
    }

    void substr(atomic_string& v, long pos, long sz) {
        long nb = pos >> base_atomic_string_size;
        long i = pos - (nb << base_atomic_string_size);
        atomic_string_element* n = head;
        while (nb > 0) {
            if (n == NULL)
                return;
            --nb;
            n = n->next;
        }
        
        if (n == NULL || i >= strlen(n->buffer))
            return;

        
        atomic_string_element* nv = v.head;
        long start = 0;
        while (sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    break;
                i = 0;
            }
            if (start == atomic_string_size) {
                if (nv->next == NULL) {
                    lock->lock();
                    if (nv->next == NULL)
                        nv->next = new atomic_string_element;
                    lock->unlock();
                }
                nv = nv->next;
                start = 0;
            }
            nv->buffer[start++] = n->buffer[i++];
            --sz;
        }
        
        nv->buffer[start] = 0;
    }
    
    bool compare(atomic_string& a) {
        if (a.isempty()) {
            if (!isempty())
                return false;
            return true;
        }

        if (head->size() != a.size())
            return false;
        
        return head->compare(a.head);
    }
    
    friend bool operator!=(atomic_string& a, atomic_string& b) {
        return !a.compare(b);
    }
    
    friend bool operator==(atomic_string& a, atomic_string& b) {
        return a.compare(b);
    }

    string value() {
        atomic_string_element* n = head;

        string v;
        while (n) {
            v += n->buffer;
            n = n->next;
        }

        return v;
    }

    bool operator==(string v) {
        atomic_string_element* n = head;
        long vi = 0;
        long i = 0;
        long sz = v.size();
        while (vi < sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    return false;
                i = 0;
            }
            if (n->buffer[i] != v[vi])
                return false;
            ++i;
            ++vi;
        }
        return true;
    }
    
    bool operator<(string v) {
        atomic_string_element* n = head;
        long vi = 0;
        long i = 0;
        long sz = v.size();
        if (!head->buffer[0] && sz)
            return true;
        
        while (vi < sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    return true;
                i = 0;
            }
            
            if (n->buffer[i] < v[vi])
                return true;
            if (n->buffer[i] > v[vi])
                return false;
            
            ++i;
            ++vi;
        }
        return false;
    }
    
    bool operator>(string v) {
        atomic_string_element* n = head;
        long vi = 0;
        long i = 0;
        long sz = v.size();
        if (!head->buffer[0] && sz)
            return false;
        
        while (vi < sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    return false;
                i = 0;
            }
            if (n->buffer[i] > v[vi])
                return true;
            if (n->buffer[i] < v[vi])
                return false;
            ++i;
            ++vi;
        }
        return false;
    }
    
    bool operator<=(string v) {
        atomic_string_element* n = head;
        long vi = 0;
        long i = 0;
        long sz = v.size();
        if (!head->buffer[0] && sz)
            return true;
        
        while (vi < sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    return true;
                i = 0;
            }
            if (n->buffer[i] < v[vi])
                return true;
            if (n->buffer[i] > v[vi])
                return false;
            ++i;
            ++vi;
        }
        return true;
    }
    
    bool operator>=(string v) {
        atomic_string_element* n = head;
        long vi = 0;
        long i = 0;
        long sz = v.size();
        if (!head->buffer[0] && sz)
            return false;
        while (vi < sz) {
            if (i == atomic_string_size) {
                n = n->next;
                if (n == NULL)
                    return true;
                i = 0;
            }
            if (n->buffer[i] > v[vi])
                return true;
            if (n->buffer[i] < v[vi])
                return false;
            ++i;
            ++vi;
        }
        return true;
    }

    bool isempty() {
        if (!head->buffer[0])
            return true;
        return false;
    }
    
    long size() {
        return head->size();
    }
    
};

//---------------------------------------------------------------------------------------------------------------------
//We create a map between our methods, which have been declared in our class below. See MethodInitialization for an example
//of how to declare a new method.
class Tamgustring;
//This typedef defines a type "stringMethod", which expose the typical parameters of a new Tamgu method implementation
typedef Tamgu* (Tamgustring::*stringMethod)(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);

//---------------------------------------------------------------------------------------------------------------------

class Tamgustring : public TamguObject {
public:
	//We export the methods that will be exposed for our new object
	//this is a static object, which is common to everyone
	//We associate the method pointers with their names in the linkedmethods map
	static Exchanging basebin_hash<stringMethod> methods;
	static Exchanging hmap<string, string> infomethods;
	static Exchanging bin_hash<unsigned long> exported;

	static Exchanging short idtype;

	//---------------------------------------------------------------------------------------------------------------------
	//This SECTION is for your specific implementation...
	//Your personal variables here...
	string value;

	//---------------------------------------------------------------------------------------------------------------------
	Tamgustring(string v, TamguGlobal* g, Tamgu* parent = NULL) : TamguObject(g, parent) {
		//Do not forget your variable initialisation
		value = v;
	}

	Tamgustring(string v) {
		//Do not forget your variable initialisation
		value = v;
	}

	//----------------------------------------------------------------------------------------------------------------------
    void Putatomicvalue(Tamgu* v) {
        locking();
        value = v->String();
        unlocking();
    }

	Exporting Tamgu* Put(Tamgu* index, Tamgu* v, short idthread);

	Tamgu* Putvalue(Tamgu* v, short idthread) {
        locking();
		value = v->String();
        unlocking();
		return this;
	}


	Exporting Tamgu* Eval(Tamgu* context, Tamgu* value, short idthread);

	Exporting Tamgu* Looptaskell(Tamgu* recipient, Tamgu* context, Tamgu* env, TamguFunctionLambda* bd, short idthread);
	Exporting Tamgu* Filter(short idthread, Tamgu* env, TamguFunctionLambda* bd, Tamgu* var, Tamgu* kcont, Tamgu* accu, Tamgu* init, bool direct);

	Tamgu* Vector(short idthread) {
		Locking _lock(this);
		return globalTamgu->EvaluateVector(value, idthread);
	}

	Tamgu* Map(short idthread) {
		Locking _lock(this);
		return globalTamgu->EvaluateMap(value, idthread);
	}

	Tamgu* Push(Tamgu* a) {
        locking();
        value += a->String();
        unlocking();
		return this;
	}

	void Pop(long i) {
		Locking _lock(this);
        long sz = size_c(value);
		if (!sz)
			return;
		if (i == -1)
			i = sz - 1;
		else
		if (i >= sz || i < 0)
			return;
		c_char_index_remove(value, i);
	}

	virtual short Type() {
		return Tamgustring::idtype;
	}

	

    static void Setidtype(TamguGlobal* global);
    
    string Typename() {
		return "string";
	}

	bool isString() {
		return true;
	}

	bool isAtom() {
		return true;
	}

	bool isValueContainer() {
		return true;
	}

	Tamgu* Atom(bool forced = false) {
        if (!globalTamgu->globalLOCK) {
            if (forced || !protect || reference)
                return globalTamgu->Providestring(value);
            return this;
        }
        
        Locking _lock(this);
        if (forced || !protect || reference)
            return globalTamgu->Providestring(value);
        return this;
	}

	void storevalue(float u) {
		string s = convertfromnumber(u);
        locking();
        value = s;
        unlocking();
	}

	void storevalue(short u) {
		string s = convertfromnumber(u);
        locking();
        value = s;
        unlocking();
	}

	void storevalue(long u) {
		string s = convertfromnumber(u);
        locking();
        value = s;
        unlocking();
	}

	void storevalue(BLONG u) {
		string s = convertfromnumber(u);
        locking();
        value = s;
        unlocking();
	}

	void storevalue(double u) {
		string s = convertfromnumber(u);
        locking();
        value = s;
        unlocking();
	}


	void Storevalue(string& u) {
        locking();
        value = u;
        unlocking();
	}

	void Storevalue(wstring& u) {
        locking();
		sc_unicode_to_utf8(value, u);
        unlocking();
	}

    void storevalue(string u) {
        locking();
        value = u;
        unlocking();
    }
    
    void storevalue(wstring u) {
        locking();
        sc_unicode_to_utf8(value, u);
        unlocking();
    }
    
	void storevalue(wchar_t u) {
		wstring w;
		w = u;
        locking();
		sc_unicode_to_utf8(value, w);
        unlocking();
	}

	void addstringto(string u) {
        locking();
		value += u;
        unlocking();
	}

	void addustringto(wstring u) {
        locking();
		s_unicode_to_utf8(value, u);
        unlocking();
	}

	void addstringto(wchar_t u) {
		wstring w;
		w = u;
        locking();
		s_unicode_to_utf8(value, w);
        unlocking();
	}

	//---------------------------------------------------------------------------------------------------------------------
	//Declaration
	//All our methods must have been declared in tamguexportedmethods... See MethodInitialization below
	bool isDeclared(short n) {
		if (exported.find(n) != exported.end())
			return true;
		return false;
	}

    Tamgu* Newpureinstance(short idthread) {
        return new Tamgustring("");
    }

	Tamgu* Newinstance(short, Tamgu* f = NULL) {
		return globalTamgu->Providestring("");
	}

	Tamgu* Newvalue(Tamgu* a, short idthread) {
		return globalTamgu->Providestring(a->String());
	}


	Exporting TamguIteration* Newiteration(bool direction);

	static void AddMethod(TamguGlobal* g, string name, stringMethod func, unsigned long arity, string infos, short returntype);

	static bool InitialisationModule(TamguGlobal* g, string version);


    void Methods(Tamgu* v) {
        string s;
        
        for (auto& it : infomethods) {
            s=it.first;
            v->storevalue(s);
        }
    }
    
	string Info(string n) {

		if (infomethods.find(n) != infomethods.end())
			return infomethods[n];
		return "Unknown method";
	}


	Tamgu* Succ();
	Tamgu* Pred();
    Tamgu* Loopin(TamguInstruction* ins, Tamgu* context, short idthread);
        
	//---------------------------------------------------------------------------------------------------------------------
	//This SECTION is for your specific implementation...
	//This is an example of a function that could be implemented for your needs.
	Tamgu* MethodSizeb(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
		Locking _lock(this);
		return globalTamgu->Provideint((long)value.size());
	}

	Tamgu* MethodSucc(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
		return Succ();
	}

	Tamgu* MethodPred(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
		return Pred();
	}

    Tamgu* MethodDoubleMetaphone(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodHash(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodReverse(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodOrd(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodBytes(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodFormat(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodBase(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodFill(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodPadding(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodParse(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodPop(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodLisp(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTags(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodScan(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodEvaluate(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTohtml(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodToxml(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodEditdistance(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodReplace(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodGetstd(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodGeterr(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodSplit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodSplite(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodMultiSplit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTokenize(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodStokenize(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodCount(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodFind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodRfind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodRemovefirst(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodRemovelast(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsutf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodNgrams(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodSlice(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodUtf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodLatin(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodDos(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodDostoutf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodLeft(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodByteposition(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodCharposition(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodRight(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodMid(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsalpha(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsconsonant(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsvowel(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIspunctuation(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsdigit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodExtract(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodEmoji(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsemoji(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsupper(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIslower(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodUpper(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodDeaccentuate(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodLower(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTrim(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTrimleft(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTrimright(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodLast(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodInsert(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodClear(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIndent(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodJamo(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsJamo(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodIsHangul(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodNormalizeHangul(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
	Tamgu* MethodTransliteration(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);

    Tamgu* MethodRead(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);

	//---------------------------------------------------------------------------------------------------------------------

	//ExecuteMethod must be implemented in order to execute our new Tamgu methods. This method is called when a TamguCallMethodMethod object
	//is returned by the Declaration method.
	Tamgu* CallMethod(short idname, Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
		//This call is a bit cryptic. It takes the method (function) pointer that has been associated in our map with "name"
		//and run it with the proper parameters. This is the right call which should be invoked from within a class definition
		return (this->*methods.get(idname))(contextualpattern, idthread, callfunc);
	}

	//---------------------------------------------------------------------------------------------------------------------
	void Clear() {
		Locking _lock(this);
		value = "";
	}

	Exporting unsigned long EditDistance(Tamgu* e);

	

	string JSonString() {
		string res;
        
        if (!globalTamgu->globalLOCK) {
            jstringing(res, value);
            return res;
        }

        Locking _lock(this);
		jstringing(res, value);
		return res;
	}

	string Bytes() {
        if (!globalTamgu->globalLOCK)
            return value;
		Locking _lock(this);
		return value;
	}

	string String() {
        if (!globalTamgu->globalLOCK)
            return value;
        
		Locking _lock(this);
		return value;
	}

	wstring UString() {
        wstring res;
        if (!globalTamgu->globalLOCK) {
            s_utf8_to_unicode(res, USTR(value), value.size());
            return res;
        }
        
		Locking _lock(this);
		s_utf8_to_unicode(res, USTR(value), value.size());
		return res;

	}

	long Integer() {
        if (!globalTamgu->globalLOCK)
            return (long)conversionintegerhexa(STR(value));
		Locking _lock(this);
		return (long)conversionintegerhexa(STR(value));
	}

	wstring Getustring(short idthread) {
		wstring res;
        if (!globalTamgu->globalLOCK) {
            s_utf8_to_unicode(res, USTR(value), value.size());
            return res;
        }
		Locking _lock(this);
		s_utf8_to_unicode(res, USTR(value), value.size());
		return res;
	}

	string Getstring(short idthread) {
        if (!globalTamgu->globalLOCK)
            return value;
        
		Locking _lock(this);
		return value;
	}

	long Getinteger(short idthread) {
		return Integer();
	}

	double Getfloat(short idthread) {
		return Float();
	}

	float Getdecimal(short idthread) {
		return Decimal();
	}

	BLONG Getlong(short idthread) {
		return Long();
	}

    long CommonSize() {
        if (!globalTamgu->globalLOCK)
            return size_c(value);
        Locking _lock(this);
        return size_c(value);
    }
    

	virtual long Size() {
        if (!globalTamgu->globalLOCK)
            return (long)value.size();
		Locking _lock(this);
		return (long)value.size();
	}

	float Decimal() {
        if (!globalTamgu->globalLOCK)
            return convertfloat(STR(value));
		Locking _lock(this);
		return convertfloat(STR(value));
	}

	double Float() {
        if (!globalTamgu->globalLOCK)
            return convertfloat(STR(value));
		Locking _lock(this);
		return convertfloat(STR(value));
	}

	bool Boolean() {
        if (!globalTamgu->globalLOCK) {
            if (value == "")
                return false;
            return true;
        }
        
		Locking _lock(this);
		if (value == "")
			return false;
		return true;
	}

	BLONG Long() {
        if (!globalTamgu->globalLOCK)
            return conversionintegerhexa(STR(value));

        Locking _lock(this);
		return conversionintegerhexa(STR(value));
	}

	Exporting Tamgu* in(Tamgu* context, Tamgu* a, short idthread);

    Tamgu* Merging(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            value += a->String();
            return this;
        }
        
        locking();
        value += a->String();
        unlocking();
        return this;
    }
    
    Tamgu* Combine(Tamgu* a) {
        locking();
        string v=a->String();
        string k;
        long zs=min(value.size(),v.size());
        for (long u=0;u<zs;u++) {
            k+=value[u];
            k+=v[u];
        }
        if (v.size()!=value.size()) {
            if (zs==v.size())
                k+=value.substr(zs,value.size()-zs);
            else
                k+=v.substr(zs,v.size()-zs);
        }
        value=k;
        unlocking();
        return this;
    }
    
	Tamgu* andset(Tamgu* a, bool autoself);
	Tamgu* xorset(Tamgu* a, bool autoself);

	//we add the current value with a
	virtual Tamgu* plus(Tamgu* a, bool itself) {
        locking();
		if (itself) {
			value += a->String();
            unlocking();
			return this;
		}

		a = globalTamgu->Providestring(value + a->String());
        unlocking();
        return a;
	}

	//we remove a from the current value
	virtual Tamgu* minus(Tamgu* a, bool itself) {
        locking();
		string v = a->String();
		size_t nb = v.size();
		size_t pos = value.find(v);
        if (pos == string::npos) {
            unlocking();
			return this;
        }

		//we keep our string up to p
		v = value.substr(0, pos);
		//then we skip the nb characters matching the size of v
		pos += nb;
		//then we concatenate with the rest of the string
		v += value.substr(pos, value.size() - pos);
		if (itself) {
			value = v;
            unlocking();
			return this;
		}
        unlocking();
		return globalTamgu->Providestring(v);
	}


	Tamgu* less(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value < a->String())
                return aTRUE;
            return aFALSE;
        }
		Locking _lock(this);
		if (value < a->String())
			return aTRUE;
		return aFALSE;
	}

	Tamgu* more(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value > a->String())
                return aTRUE;
            return aFALSE;
        }
		Locking _lock(this);
		if (value > a->String())
			return aTRUE;
		return aFALSE;
	}

	Tamgu* same(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value == a->String())
                return aTRUE;
            return aFALSE;
        }
		Locking _lock(this);
		if (value == a->String())
			return aTRUE;
		return aFALSE;
	}

	Tamgu* different(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value != a->String())
                return aTRUE;
            return aFALSE;
        }
		Locking _lock(this);
		if (value != a->String())
			return aTRUE;
		return aFALSE;
	}

	Tamgu* lessequal(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value <= a->String())
                return aTRUE;
            return aFALSE;
        }
		Locking _lock(this);
		if (value <= a->String())
			return aTRUE;
		return aFALSE;
	}

	Tamgu* moreequal(Tamgu* a) {
        if (!globalTamgu->globalLOCK) {
            if (value >= a->String())
                return aTRUE;
            return aFALSE;
        }
        
		Locking _lock(this);
		if (value >= a->String())
			return aTRUE;
		return aFALSE;
	}
};

class Tamgustringbuff : public Tamgustring {
public:
	long idx;
	bool used;

	Tamgustringbuff(long i) : Tamgustring("") {
		//Do not forget your variable initialisation
		idx = i;
		used = false;
	}

    bool Candelete() {
        return false;
    }

	void Resetreference(short r) {
        reference -= r;
        if (reference <= 0) {
            if (!protect) {
                reference = 0;
                protect = true;
                
                value = "";
                used = false;
                globalTamgu->sempties.push_back(idx);
            }
        }
	}

};

class TamguIterationstring : public TamguIteration {
public:

	agnostring ref;

	TamguIterationstring(string& v, bool d, TamguGlobal* g = NULL) : ref(v), TamguIteration(d, g) {
		if (d)
			ref = ref.revert();
	}

	Tamgu* Key() {
		return globalTamgu->Provideint((long)ref.getcharpos());
	}

	Tamgu* Value() {
		return globalTamgu->Providestring(ref.current());
	}

	string Keystring() {
		
		return convertfromnumber((long)ref.getcharpos());
		
	}

	wstring Valueustring() {
		return ref.wcurrent();
	}

	string Valuestring() {
		return ref.current();
	}

	long Keyinteger() {
		return (long)ref.getcharpos();
	}

	long Valueinteger() {
		string s = ref.current();
		return conversionintegerhexa(STR(s));
	}

	double Keyfloat() {
		return (double)ref.getcharpos();
	}

	double Valuefloat() {
		string s = ref.current();
		return convertfloat(STR(s));
	}

	void Next() {
		ref.following();
	}

	Tamgu* End() {
        return booleantamgu[ref.end()];
	}

	Tamgu* Begin() {
		ref.begin();
		return aTRUE;
	}


};

class TamguLoopString : public Tamgustring {
public:

	vector<string> interval;
	long position;
	Tamgu* function;

	TamguLoopString(TamguGlobal* g) : Tamgustring("") {

		position = 0;
		function = NULL;
	}

    bool isLoop() {
        return true;
    }
    

	void Callfunction();

	void Addfunction(Tamgu* f) {
		function = f;
	}

	short Type() {
		return a_sloop;
	}

    unsigned long CallBackArity() {
        return P_TWO;
    }

	Tamgu* Put(Tamgu*, Tamgu*, short);
	Tamgu* Vector(short idthread);
    Tamgu* Putvalue(Tamgu* ke, short idthread) {
        return Put(aNULL, ke, idthread);
    }

	long Size() {
		return interval.size();
	}

	void Next() {
		if (interval.size() == 0)
			return;

		position++;
		if (position >= interval.size()) {
			if (function != NULL)
				Callfunction();
			position = 0;
		}
        
        Locking _lock(this);
		value = interval[position];
	}

	Tamgu* plusplus() {
		if (interval.size() == 0)
			return this;
		position++;
		if (position >= interval.size()) {
			if (function != NULL)
				Callfunction();
			position = 0;
		}
        Locking _lock(this);
		value = interval[position];
		return this;
	}

	Tamgu* minusminus() {
		if (interval.size() == 0)
			return this;
		position--;
		if (position < 0) {
			if (function != NULL)
				Callfunction();
			position = interval.size() - 1;
		}
        Locking _lock(this);
		value = interval[position];
		return this;
	}

	Tamgu* Atom(bool forced) {
		return this;
	}

	bool duplicateForCall() {
		return false;
	}

	Tamgu* Newinstance(short, Tamgu* f = NULL) {
		TamguLoopString* a = new TamguLoopString(globalTamgu);
		a->function = f;
		return a;
	}

	Tamgu* andset(Tamgu* a, bool autoself);
	Tamgu* xorset(Tamgu* a, bool autoself);

	Tamgu* plus(Tamgu* b, bool autoself) {
		if (interval.size() == 0)
			return this;

		if (autoself) {
			position += b->Integer();

			position = abs(position) % interval.size();

            Locking _lock(this);
			value = interval[position];
			return this;
		}

		return Tamgustring::plus(b, autoself);
	}

	Tamgu* minus(Tamgu* b, bool autoself) {
		if (interval.size() == 0)
			return this;

		if (autoself) {
			position -= b->Integer();

			position = (interval.size() - abs(position)) % interval.size();

            Locking _lock(this);
			value = interval[position];
			return this;
		}

		return Tamgustring::minus(b, autoself);
	}
};



//---------------------------------------------------------------------------------------------------------------------
    //We create a map between our methods, which have been declared in our class below. See MethodInitialization for an example
    //of how to declare a new method.
class Tamgua_string;
    //This typedef defines a type "stringMethod", which expose the typical parameters of a new Tamgu method implementation
typedef Tamgu* (Tamgua_string::*a_stringMethod)(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);

    //---------------------------------------------------------------------------------------------------------------------

class Tamgua_string : public TamguReference {
public:
        //We export the methods that will be exposed for our new object
        //this is a static object, which is common to everyone
        //We associate the method pointers with their names in the linkedmethods map
    static Exchanging basebin_hash<a_stringMethod> methods;
    static Exchanging hmap<string, string> infomethods;
    static Exchanging bin_hash<unsigned long> exported;
    
    static Exchanging short idtype;
    
    //---------------------------------------------------------------------------------------------------------------------
    //This SECTION is for your specific implementation...
    //Your personal variables here...
    atomic_string value;
    
    //---------------------------------------------------------------------------------------------------------------------
    Tamgua_string(string v, TamguGlobal* g, Tamgu* parent = NULL) : TamguReference(g, parent) {
        //Do not forget your variable initialisation
        value = v;
    }

    Tamgua_string(char c) {
        value.head->buffer[0] = c;
        value.head->buffer[1] = 0;
    }

    Tamgua_string(string v) {
            //Do not forget your variable initialisation
        value = v;
    }
    
    Tamgua_string(atomic_string& v) {
            //Do not forget your variable initialisation
        value = v;
    }
    
    //----------------------------------------------------------------------------------------------------------------------
    void Putatomicvalue(Tamgu* v) {
        value = v->String();
    }
    
    Exporting Tamgu* Put(Tamgu* index, Tamgu* v, short idthread);
    
    Tamgu* Putvalue(Tamgu* v, short idthread) {
        value = v->String();
        return this;
    }
    
    
    Exporting Tamgu* Eval(Tamgu* context, Tamgu* value, short idthread);
    
    Exporting Tamgu* Looptaskell(Tamgu* recipient, Tamgu* context, Tamgu* env, TamguFunctionLambda* bd, short idthread);
    Exporting Tamgu* Filter(short idthread, Tamgu* env, TamguFunctionLambda* bd, Tamgu* var, Tamgu* kcont, Tamgu* accu, Tamgu* init, bool direct);
    
    Tamgu* Vector(short idthread) {
        string v = value.value();
        return globalTamgu->EvaluateVector(v, idthread);
    }
    
    Tamgu* Map(short idthread) {
        string v = value.value();
        return globalTamgu->EvaluateMap(v, idthread);
    }
    
    Tamgu* Push(Tamgu* a) {
        value.concat(a->String());
        return this;
    }
    
    void Pop(long i) {
        if (value.isempty())
            return;
        string v = value.value();
        long sz = size_c(v);
        if (!sz)
            return;
        if (i == -1)
            i = sz - 1;
        else
            if (i >= sz || i < 0)
                return;
        c_char_index_remove(v, i);
        value = v;
    }
    
    short Type() {
        return Tamgua_string::idtype;
    }
    
    string Typename() {
        return "a_string";
    }
    
    bool isString() {
        return true;
    }
    
    bool isAtom() {
        return true;
    }
    
    bool isValueContainer() {
        return true;
    }
    
    Tamgu* Atom(bool forced = false) {
        if (forced || !protect || reference)
            return new Tamgua_string(value);
        return this;
    }
    
    void storevalue(float u) {
        value = convertfromnumber(u);
    }
    
    void storevalue(short u) {
        value = convertfromnumber(u);
    }
    
    void storevalue(long u) {
        value = convertfromnumber(u);
    }
    
    void storevalue(BLONG u) {
        value = convertfromnumber(u);
    }
    
    void storevalue(double u) {
        value = convertfromnumber(u);
    }
    
    
    void Storevalue(string& u) {
        value = u;
    }
    
    void Storevalue(wstring& u) {
        string v;
        sc_unicode_to_utf8(v, u);
        value = v;
    }
    
    void storevalue(string u) {
        value = u;
    }
    
    void storevalue(wstring u) {
        string v;
        sc_unicode_to_utf8(v, u);
        value = v;
    }
    
    void storevalue(wchar_t u) {
        wstring w;
        w = u;
        string v;
        sc_unicode_to_utf8(v, w);
        value = v;
    }
    
    void addstringto(string u) {
        value.concat(u);
    }
    
    void addustringto(wstring u) {
        string v;
        s_unicode_to_utf8(v, u);
        value = v;
    }
    
    void addstringto(wchar_t u) {
        wstring w;
        w = u;
        string v;
        s_unicode_to_utf8(v, w);
        value = v;
    }
    
        //---------------------------------------------------------------------------------------------------------------------
        //Declaration
        //All our methods must have been declared in tamguexportedmethods... See MethodInitialization below
    bool isDeclared(short n) {
        if (exported.find(n) != exported.end())
            return true;
        return false;
    }
    
    Tamgu* Newinstance(short, Tamgu* f = NULL) {
        return new Tamgua_string("");
    }
    
    Tamgu* Newvalue(Tamgu* a, short idthread) {
        return new Tamgua_string(a->String());
    }
    
    
    Exporting TamguIteration* Newiteration(bool direction);
    
    static void AddMethod(TamguGlobal* g, string name, a_stringMethod func, unsigned long arity, string infos, short returntype);
    
    static bool InitialisationModule(TamguGlobal* g, string version);
    
    
    void Methods(Tamgu* v) {
        string s;
        
        for (auto& it : infomethods) {
            s=it.first;
            v->storevalue(s);
        }
    }
    
    string Info(string n) {
        
        if (infomethods.find(n) != infomethods.end())
            return infomethods[n];
        return "Unknown method";
    }
    
    
    Tamgu* Succ();
    Tamgu* Pred();
    Tamgu* Loopin(TamguInstruction* ins, Tamgu* context, short idthread);
    
        //---------------------------------------------------------------------------------------------------------------------
        //This SECTION is for your specific implementation...
        //This is an example of a function that could be implemented for your needs.
    Tamgu* MethodSizeb(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        return globalTamgu->Provideint((long)value.size());
    }
    
    Tamgu* MethodSucc(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        return Succ();
    }
    
    Tamgu* MethodPred(Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
        return Pred();
    }
    
    Tamgu* MethodDoubleMetaphone(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodHash(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodReverse(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodOrd(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodBytes(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodFormat(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodBase(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodFill(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodPadding(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodParse(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodPop(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodEditdistance(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodReplace(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodSplit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodSplite(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodMultiSplit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodTokenize(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodStokenize(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodCount(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodFind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodRfind(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodRemovefirst(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodRemovelast(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsutf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodNgrams(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodSlice(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodUtf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodLatin(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodDos(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodDostoutf8(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodLeft(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodByteposition(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodCharposition(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodRight(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodMid(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsalpha(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsconsonant(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsvowel(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIspunctuation(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsdigit(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodExtract(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodEmoji(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsemoji(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsupper(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIslower(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodUpper(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodDeaccentuate(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodLower(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodTrim(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodTrimleft(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodTrimright(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodLast(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodInsert(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodClear(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodJamo(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsJamo(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodIsHangul(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodNormalizeHangul(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);
    Tamgu* MethodTransliteration(Tamgu* contextualpattern, short idthread, TamguCall* callfunc);

    
        //---------------------------------------------------------------------------------------------------------------------
    
        //ExecuteMethod must be implemented in order to execute our new Tamgu methods. This method is called when a TamguCallMethodMethod object
        //is returned by the Declaration method.
    Tamgu* CallMethod(short idname, Tamgu* contextualpattern, short idthread, TamguCall* callfunc) {
            //This call is a bit cryptic. It takes the method (function) pointer that has been associated in our map with "name"
            //and run it with the proper parameters. This is the right call which should be invoked from within a class definition
        return (this->*methods.get(idname))(contextualpattern, idthread, callfunc);
    }
    
        //---------------------------------------------------------------------------------------------------------------------
    void Clear() {
        value.clear();
    }
    
    Exporting unsigned long EditDistance(Tamgu* e);
    
    string JSonString() {
        string res;
        string v = value.value();
        jstringing(res, v);
        return res;
    }
    
    string Bytes() {
        return value.value();
    }
    
    string String() {
        return value.value();
    }
    
    wstring UString() {
        wstring res;
        string v = value.value();
        s_utf8_to_unicode(res, USTR(v), v.size());
        return res;
        
    }
    
    long Integer() {
        string v = value.value();
        return (long)conversionintegerhexa(STR(v));
    }
    
    wstring Getustring(short idthread) {
        wstring res;
        string v = value.value();
        s_utf8_to_unicode(res, USTR(v), v.size());
        return res;
    }
    
    string Getstring(short idthread) {
        return value.value();
    }
    
    long Getinteger(short idthread) {
        return Integer();
    }
    
    double Getfloat(short idthread) {
        return Float();
    }
    
    float Getdecimal(short idthread) {
        return Decimal();
    }
    
    BLONG Getlong(short idthread) {
        return Long();
    }
    
    long CommonSize() {
        string v = value.value();
        return size_c(v);
    }
    
    
    long Size() {
        return (long)value.size();
    }
    
    float Decimal() {
        string v = value.value();
        return convertfloat(STR(v));
    }
    
    double Float() {
        string v = value.value();
        return convertfloat(STR(v));
    }
    
    bool Boolean() {
        return !value.isempty();
    }
    
    BLONG Long() {
        string v = value.value();
        return conversionintegerhexa(STR(v));
    }
    
    Exporting Tamgu* in(Tamgu* context, Tamgu* a, short idthread);
    
    Tamgu* Merging(Tamgu* a) {
        value.concat(a->String());
        return this;
    }
    
    Tamgu* Combine(Tamgu* a) {
        string vl = value.value();
        string v=a->String();
        string k;
        long zs=min(vl.size(),v.size());
        for (long u=0;u<zs;u++) {
            k+=vl[u];
            k+=v[u];
        }
        if (v.size()!=value.size()) {
            if (zs==v.size())
                k+=vl.substr(zs,vl.size()-zs);
            else
                k+=v.substr(zs,v.size()-zs);
        }
        value = k;
        return this;
    }
    
    Tamgu* andset(Tamgu* a, bool autoself);
    Tamgu* xorset(Tamgu* a, bool autoself);
    
        //we add the current value with a
    Tamgu* plus(Tamgu* a, bool itself) {
        if (itself) {
            value.concat(a->String());
            return this;
        }

        Tamgua_string* n = new Tamgua_string(value);
        n->value.concat(a->String());
        return n;
    }
    
        //we remove a from the current value
    Tamgu* minus(Tamgu* a, bool itself) {
        string v = a->String();
        size_t nb = v.size();
        string vl = value.value();
        size_t pos = vl.find(v);
        if (pos == string::npos)
            return this;
        
            //we keep our string up to p
        v = vl.substr(0, pos);
            //then we skip the nb characters matching the size of v
        pos += nb;
            //then we concatenate with the rest of the string
        v += vl.substr(pos, vl.size() - pos);
        if (itself) {
            value = v;
            return this;
        }
        return new Tamgua_string(v);
    }
    
    
    Tamgu* less(Tamgu* a) {
        if (value < a->String())
            return aTRUE;
        return aFALSE;
    }
    
    Tamgu* more(Tamgu* a) {
        if (value > a->String())
            return aTRUE;
        return aFALSE;
    }
    
    Tamgu* same(Tamgu* a) {
        if (value == a->String())
            return aTRUE;
        return aFALSE;
    }
    
    Tamgu* different(Tamgu* a) {
        if (value == a->String())
            return aFALSE;
        return aTRUE;
    }
    
    Tamgu* lessequal(Tamgu* a) {
        if (value <= a->String())
            return aTRUE;
        return aFALSE;
    }
    
    Tamgu* moreequal(Tamgu* a) {
        if (value >= a->String())
            return aTRUE;
        return aFALSE;
    }
};

#endif









