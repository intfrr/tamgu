/*
 *  Tamgu (탐구)
 *
 * Copyright 2019-present NAVER Corp.
 * under BSD 3-clause
 */
/* --- CONTENTS ---
 Project    : Tamgu (탐구)
 Version    : See tamgu.cxx for the version number
 filename   : jagrgx.h
 Date       : 2017/09/01
 Purpose    :  
 Programmer : Claude ROUX (claude.roux@naverlabs.com)
 Reviewer   :
 */

#ifndef jagrgx_h
#define jagrgx_h

#include "jagvecte.h"

typedef enum{ an_epsilon=0, an_end=1, an_remove=2, an_negation=4, an_mandatory=8, an_error=12, an_rule=16, an_final=32, an_beginning=64, an_ending=128} an_flag;
typedef enum{an_meta=1, an_automaton, an_regex, an_token, an_any, an_lemma, an_label, an_orlabels, an_andlabels} an_type;

typedef enum{aut_reg=1,aut_reg_plus,aut_reg_star,
    aut_meta, aut_meta_plus, aut_meta_star,
    aut_any,aut_any_plus,aut_any_star,
    aut_ocrl_brk, aut_ccrl_brk, aut_ccrl_brk_plus, aut_ccrl_brk_star,
    aut_obrk, aut_cbrk, aut_cbrk_plus, aut_cbrk_star,
    aut_opar, aut_cpar, aut_negation
}
aut_actions;

//------------------------character automatons---------------------------------------------------
class Au_automaton;
class Au_automatons;
class Au_automate;
//-----------------------------------------------------------------------------------------------

class Au_state;
class Au_any {
public:
    bool vero;
    an_type type;
    
    Au_any(unsigned char t) {
        type=(an_type)t;
        vero = true;
    }
    
    virtual bool same(Au_any* a) {
        if (a->Type()==type && a->vero == vero)
            return true;
        return false;
    }
    
    an_type Type() {
        return type;
    }
    
    virtual char compare(TAMGUCHAR c) {
        return vero;
    }
    
    void setvero(bool neg) {
        vero = !neg;
    }
};

class Au_char : public Au_any {
public:
    wchar_t action;
    
    Au_char(wchar_t a, unsigned char t) : Au_any(t) {
        action=a;
    }
    
    bool same(Au_any* a) {
        if (a->Type()==type && a->vero == vero && ((Au_char*)a)->action==action)
            return true;
        return false;
    }

    char compare(TAMGUCHAR c) {
        if (action==c)
            return vero;
        return !vero;
    }
    
};

class Au_epsilon : public Au_any {
public:
    
    Au_epsilon()  : Au_any(an_epsilon) {}
    
    bool same(Au_any* a) {
        if (a->Type()==an_epsilon && a->vero == vero)
            return true;
        return false;
    }

    char compare(TAMGUCHAR c) {
        return 2;
    }
    
};

class Au_meta : public Au_any {
public:
    
    wchar_t action;
    
    Au_meta(uchar a, unsigned char t) : Au_any(t) {
        action=a;
    }
    
    bool same(Au_any* a) {
        if (a->Type()==type && a->vero == vero && ((Au_meta*)a)->action==action)
            return true;
        return false;
    }

    //CHSacdnprsx
    char compare(TAMGUCHAR car) {
        
        switch(action) {
            case 'C':
                if (c_is_upper(car))
                    return vero;
                return !vero;
            case 'e':
                if (c_is_emoji(car))
                    return vero;
                return !vero;
            case 'E':
                if (c_is_emojicomp(car))
                    return vero;
                return !vero;
            case 'H':
                if (c_is_hangul(car))
                    return vero;
                return !vero;
            case 'S':
                if (car <= 32 || car == 160)
                    return vero;
                return !vero;
            case 'a':
                if (car=='_' || c_is_alpha(car))
                    return vero;
                return !vero;
            case 'c':
                if (c_is_lower(car))
                    return vero;
                return !vero;
            case 'd':
                if (c_is_digit(car))
                    return vero;
                return !vero;
            case 'n':
                if (car == 160)
                    return vero;
                return !vero;
            case 'p':
                if (c_is_punctuation(car))
                    return vero;
                return !vero;
            case 'r':
                if (car == 10 || car == 13)
                    return vero;
                return !vero;
            case 's':
                if (car == 9 || car == 32 || car == 160)
                    return vero;
                return !vero;
            case 'x': //hexadecimal character
                if ((car>='A' && car <= 'F') || (car>='a' && car <= 'f') || (car >= '0' && car <= '9'))
                    return vero;
                return !vero;
            default:
                if (action == car)
                    return vero;
                return !vero;
        }
    }
};

class Au_arc {
public:
    Au_any* action;
    Au_state* state;
    unsigned char mark;
    
    Au_arc(Au_any* a) {
        action=a;
        state=NULL;
        mark=false;
    }
    
    ~Au_arc() {
        delete action;
    }
    
    an_type Type() {
        return action->Type();
    }

    bool same(Au_arc* a) {
        return action->same(a->action);
    }
    
    bool checkfinalepsilon();

    bool find(wstring& w, wstring& sep, long i, vector<long>& res);

};

class Au_state {
public:
    VECTE<Au_arc*> arcs;
    uchar status;
    unsigned char mark;
    
    Au_state() {
        status=0;
        mark=false;
    }

    void storerulearcs(hmap<long,bool>& rules);

    virtual long idx() {return -1;}
    bool isend() {
        if ((status&an_end)==an_end)
            return true;
        return false;
    }
    
    void removeend() {
        status &= ~an_end;
    }

    bool isfinal() {
        if (arcs.last == 0)
            return true;
        
        if ((status&an_final)==an_final)
            return true;
        return false;
    }
    
    bool isrule() {
        if ((status&an_rule)==an_rule)
            return true;
        return false;
    }
    
    bool match(wstring& w, long i);
    bool find(wstring& w, wstring& sep, long i, vector<long>& res);

    long loop(wstring& w, long i);
    
    void removeepsilon();
    void addrule(Au_arc*);
    void merge(Au_state*);
    
    Au_arc* build(Au_automatons* g, wstring& token, uchar type, Au_state* common, bool nega);
    Au_state* build(Au_automatons* g, long i,vector<wstring>& tokens, vector<aut_actions>& types, Au_state* common);
};

class Au_state_final : public Au_state {
public:
    
    long rule;
    
    Au_state_final(long r) {
        rule=r;
        status=an_rule;
    }

    long idx() {return rule;}

};

class Au_automaton {
public:
    
    Au_state* first;
    
    Au_automaton() {
        first=NULL;
    }

    Au_automaton(string rgx);
    
    bool match(string& w);
    bool match(wstring& w);
    bool search(wstring& w);
    long find(wstring& w);
    long find(wstring& w, long i);


    bool search(wstring& w, long& first, long& last, long init = 0);
    bool search(string& w, long& first, long& last, long init = 0);
    bool searchc(string& w, long& first, long& last, long& firstc, long init = 0);
    bool searchraw(string& w, long& first, long& last, long init = 0);

    bool searchlast(wstring& w, long& first, long& last, long init = 0);
    bool searchlast(string& w, long& first, long& last, long init = 0);
    bool searchlastc(string& w, long& first, long& last, long& firstc, long init = 0);
    bool searchlastraw(string& w, long& first, long& last, long init = 0);

    bool bytesearch(wstring& w, long& first, long& last);
    void bytesearchall(wstring& w, vector<long>& res);

    void searchall(wstring& w, vector<long>& res, long init = 0);
    void searchall(string& w, vector<long>& res, long init = 0);
    void searchallraw(string& w, vector<long>& res, long init = 0);
    
    void find(string& w, string& sep, vector<long>& res);
    void find(wstring& w, wstring& sep, vector<long>& res);
    virtual bool parse(wstring& rgx, Au_automatons* automatons=NULL);
    
};


class Au_automatons {
public:
    VECTE<Au_state*> states;
    VECTE<Au_arc*> arcs;

    Au_state* state() {
        Au_state* s=new Au_state;
        states.push_back(s);
        return s;
    }

    Au_state* statefinal(long r) {
        Au_state_final* s=new Au_state_final(r);
        states.push_back(s);
        return s;
    }

    Au_arc* arc(Au_any* a, Au_state* s=NULL) {
        Au_arc* ac=new Au_arc(a);
        arcs.push_back(ac);
        if (s==NULL)
            ac->state=state();
        else
            ac->state=s;
        return ac;
    }

    void clean() {
        states.wipe();
        arcs.wipe();
    }

    void clean(long s, long a) {
        long i,ii;
        //We delete the states marked for destruction...
        for (i=s;i<states.size();i++) {
            if (states.vecteur[i] != NULL && states.vecteur[i]->mark == an_remove) {
                delete states.vecteur[i];
                states.vecteur[i]=NULL;
            }
        }


        //Compacting
        //We remove the NULL...
        for (i=s;i<states.size();i++) {
            if (states.vecteur[i]==NULL) {
                ii=i;
                while (ii<states.last && states.vecteur[ii]==NULL) ii++;
                if (ii==states.last) {
                    states.last=i;
                    break;
                }
                states.vecteur[i]=states.vecteur[ii];
                states.vecteur[ii]=NULL;
            }
        }

        //We delete the arcs marked for destruction...
        for (i=a;i<arcs.size();i++) {
            if (arcs.vecteur[i] != NULL && arcs.vecteur[i]->mark == an_remove) {
                delete arcs.vecteur[i];
                arcs.vecteur[i]=NULL;
            }
        }

        //Compacting
        //We remove the NULL...
        for (i=a;i<arcs.size();i++) {
            if (arcs.vecteur[i]==NULL) {
                ii=i;
                while (ii<arcs.last && arcs.vecteur[ii]==NULL) ii++;
                if (ii==arcs.last) {
                    arcs.last=i;
                    break;
                }
                arcs.vecteur[i]=arcs.vecteur[ii];
                arcs.vecteur[ii]=NULL;
            }
        }
    }

    void clearmarks() {
        long i;
        for (i=0;i<states.last;i++) {
            if (states.vecteur[i]!=NULL)
                states.vecteur[i]->mark=false;
        }
        for (i=0;i<arcs.last;i++) {
            if (arcs.vecteur[i] != NULL)
                arcs.vecteur[i]->mark=false;
        }
    }

    void clear(long s, long a) {
        long i;
        for (i=s;i<states.last;i++) {
            if (states.vecteur[i]!=NULL)
                states.vecteur[i]->mark=false;
        }
        for (i=a;i<arcs.last;i++) {
            if (arcs.vecteur[i] != NULL)
                arcs.vecteur[i]->mark=false;
        }
    }

    void boundaries(long& s, long& a) {
        s=states.size();
        a=arcs.size();
    }

    ~Au_automatons() {
        states.wipe();
        arcs.wipe();
    }
};

class Au_automate : public Au_automaton {
public:
    Au_automatons garbage;

    Au_automate() {
        first=NULL;
    }

    Au_automate(string rgx);
    Au_automate(wstring& rgx);

    bool compile(wstring& rgx) {
        return parse(rgx, &garbage);
    }

    bool compiling(wstring& rgx,long feature);
};


class Jag_automaton :  public Au_automate {
public:
    
    wstring regularexpression;
    
    Jag_automaton(wstring& rgx) : Au_automate(rgx) {
        regularexpression = rgx;
    }

};

#endif

