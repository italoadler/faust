#include "sigPromotion.hh"
#include <stdlib.h>
#include <cstdlib>
#include <map>
#include "global.hh"
#include "ppsig.hh"
#include "property.hh"
#include "signals.hh"
#include "sigtyperules.hh"
#include "tlib.hh"
#include "tree.hh"

/*
## smartIntCast[S] : adds an intCast(S) only if needed

    smartIntCast[S]     = intCast(S)    when type(S) = float
    smartIntCast[S]     = S             otherwise
*/

Tree SignalPromotion::smartIntCast(Type t, Tree sig)
{
    if (t->nature() == kReal) {
        return sigIntCast(sig);
    } else {
        return sig;
    }
}

/*
## smartFloatCast[S] : adds a floatCast(S) only if needed

    smartFloatCast[S]   = floatCast(S)      when type(S) = int
                        = S                 otherwise

*/
Tree SignalPromotion::smartFloatCast(Type t, Tree sig)
{
    if (t->nature() == kInt) {
        return sigFloatCast(sig);
    } else {
        return sig;
    }
}

/********************************************************************
SignalPromotion::transformation(Tree sig) :

Adds explicite int or float cast when needed. This is needed prior
to any optimisations to avoid to scramble int and float expressions
**********************************************************************/

Tree SignalPromotion::transformation(Tree sig)
{
    int    i;
    double r;
    Tree   c, sel, x, y, z, u, v, var, le, label, id, ff, largs, type, name, file, sf;

    if (isSigFixDelay(sig, x, y)) {
        return sigFixDelay(self(x), smartIntCast(getCertifiedSigType(y), self(y)));
    }

    // Binary operations
    // kAdd, kSub, kMul, kDiv, kRem, kLsh, kRsh, kGT, kLT, kGE, kLE, kEQ, kNE, kAND, kOR, kXOR };
    else if (isSigBinOp(sig, &i, x, y)) {
        Type tx = getCertifiedSigType(x);
        Type ty = getCertifiedSigType(y);

        switch (i) {
            case kAdd:
            case kSub:
            case kMul:
            case kRem:
            case kGT:
            case kLT:
            case kGE:
            case kLE:
            case kEQ:
            case kNE:
                if (tx->nature() == ty->nature()) {
                    // same nature => no promotion needed
                    return sigBinOp(i, self(x), self(y));
                } else {
                    // float promotion needed
                    return sigBinOp(i, smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
                }

            case kDiv:
                // the result of a division is always a float
                return sigBinOp(i, smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
            default:
                // TODO: no clear rules here
                return sigBinOp(i, self(x), self(y));
        }
    }

    // Select2 and Select3
    else if (isSigSelect2(sig, sel, x, y)) {
        Type ts = getCertifiedSigType(sel);
        Type tx = getCertifiedSigType(x);
        Type ty = getCertifiedSigType(y);

        if (tx->nature() == ty->nature()) {
            // same nature => no promotion needed
            return sigSelect2(smartIntCast(ts, self(sel)), self(x), self(y));
        } else {
            // float promotion needed
            return sigSelect2(smartIntCast(ts, self(sel)), smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
        }
    } else if (isSigSelect3(sig, sel, x, y, z)) {
        Type ts = getCertifiedSigType(sel);
        Type tx = getCertifiedSigType(x);
        Type ty = getCertifiedSigType(y);
        Type tz = getCertifiedSigType(z);

        if ((tx->nature() == ty->nature()) && (tx->nature() == tz->nature())) {
            // same nature => no promotion needed
            return sigSelect3(smartIntCast(ts, self(sel)), self(x), self(y), self(z));
        } else {
            // float promotion needed
            return sigSelect3(smartIntCast(ts, self(sel)), smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)),
                              smartFloatCast(tz, self(z)));
        }
    }

    // Int and Float Cast
    else if (isSigIntCast(sig, x)) {
        return smartIntCast(getCertifiedSigType(x), self(x));
    } else if (isSigFloatCast(sig, x)) {
        return smartFloatCast(getCertifiedSigType(x), self(x));
    }

    // Other cases => identity transformation
    else {
        return SignalIdentity::transformation(sig);
    }
}

#if 0


Tree SignalPromotion::transformation(Tree sig)
{
    int    i;
    double r;
    Tree   c, sel, x, y, z, u, v, var, le, label, id, ff, largs, type, name, file, sf;

    if (getUserData(sig)) {
        vector<Tree> newBranches;
        for (Tree b : sig->branches()) {
            newBranches.push_back(self(b));
        }
        return tree(sig->node(), newBranches);
    } else if (isSigInt(sig, &i)) {
        return sig;
    } else if (isSigReal(sig, &r)) {
        return sig;
    } else if (isSigWaveform(sig)) {
        return sig;
    } else if (isSigInput(sig, &i)) {
        return sig;
    } else if (isSigOutput(sig, &i, x)) {
        return sigOutput(i, self(x));
    } else if (isSigDelay1(sig, x)) {
        return sigDelay1(self(x));
    } else if (isSigFixDelay(sig, x, y)) {
        return sigFixDelay(self(x), smartIntCast(getCertifiedSigType(y), self(y)));
    } else if (isSigPrefix(sig, x, y)) {
        return sigPrefix(self(x), self(y));
    } else if (isSigIota(sig, x)) {
        return sigIota(self(x));
    }

    // Binary operations
    // kAdd, kSub, kMul, kDiv, kRem, kLsh, kRsh, kGT, kLT, kGE, kLE, kEQ, kNE, kAND, kOR, kXOR };
    else if (isSigBinOp(sig, &i, x, y)) {
        Type tx = getCertifiedSigType(x);
        Type ty = getCertifiedSigType(y);

        switch (i) {
            case kAdd:
            case kSub:
            case kRem:
            case kGT:
            case kLT:
            case kGE:
            case kLE:
            case kEQ:
            case kNE:
                if (tx->nature() != ty->nature()) {
                    // we have to do a float promotion on x or y
                    return sigBinOp(i, smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
                } else {
                    // same types => no promotion needed
                    return sigBinOp(i, self(x), self(y));
                }
            case kMul:
                if (tx->nature() != ty->nature()) {
                    // we have to do a float promotion on x or y
                    Tree r = sigBinOp(i, smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
                    /*                     cerr << "kMul Promotion of " << ppsig(sig) << "\n"
                                             << "Result in " << ppsig(r) << endl;
                     */
                    return r;
                } else {
                    // same types => no promotion needed
                    Tree r = sigBinOp(i, self(x), self(y));
                    /*                     cerr << "kMul (no promotion) of " << ppsig(sig) << "\n"
                                             << "Result in " << ppsig(r) << endl;
                     */
                    return r;
                }

            case kDiv:
                // the result of a division is always a float
                return sigBinOp(i, smartFloatCast(tx, self(x)), smartFloatCast(ty, self(y)));
            default:
                // TODO: no clear rules here
                return sigBinOp(i, self(x), self(y));
        }
    }

    // Foreign functions
    else if (isSigFFun(sig, ff, largs)) {
        return sigFFun(ff, mapself(largs));
    } else if (isSigFConst(sig, type, name, file)) {
        return sig;
    } else if (isSigFVar(sig, type, name, file)) {
        return sig;
    }

    // Tables
    else if (isSigTable(sig, id, x, y)) {
        return sigTable(id, self(x), self(y));
    } else if (isSigWRTbl(sig, id, x, y, z)) {
        return sigWRTbl(id, self(x), self(y), self(z));
    } else if (isSigRDTbl(sig, x, y)) {
        return sigRDTbl(self(x), self(y));
    }

    // Doc
    else if (isSigDocConstantTbl(sig, x, y)) {
        return sigDocConstantTbl(self(x), self(y));
    } else if (isSigDocWriteTbl(sig, x, y, u, v)) {
        return sigDocWriteTbl(self(x), self(y), self(u), self(v));
    } else if (isSigDocAccessTbl(sig, x, y)) {
        return sigDocAccessTbl(self(x), self(y));
    }

    // Select2 and Select3
    else if (isSigSelect2(sig, sel, x, y)) {
        return sigSelect2(self(sel), self(x), self(y));
    } else if (isSigSelect3(sig, sel, x, y, z)) {
        return sigSelect3(self(sel), self(x), self(y), self(z));
    }

    // Table sigGen
    else if (isSigGen(sig, x)) {
        if (fVisitGen) {
            return sigGen(self(x));
        } else {
            return sig;
        }
    }

    // recursive signals
    else if (isProj(sig, &i, x)) {
        return sigProj(i, self(x));
    } else if (isRec(sig, var, le)) {
        if (isNil(le)) {
            // we are already visiting this recursive group
            return sig;
        } else {
            // first visit
            rec(var, gGlobal->nil);  // to avoid infinite recursions
            return rec(var, mapself(le));
        }
    }

    // Int and Float Cast
    else if (isSigIntCast(sig, x)) {
        return smartIntCast(getCertifiedSigType(x), self(x));
    } else if (isSigFloatCast(sig, x)) {
        return smartFloatCast(getCertifiedSigType(x), self(x));
    }

    // UI
    else if (isSigButton(sig, label)) {
        return sig;
    } else if (isSigCheckbox(sig, label)) {
        return sig;
    } else if (isSigVSlider(sig, label, c, x, y, z)) {
        return sigVSlider(label, self(c), self(x), self(y), self(z));
    } else if (isSigHSlider(sig, label, c, x, y, z)) {
        return sigHSlider(label, self(c), self(x), self(y), self(z));
    } else if (isSigNumEntry(sig, label, c, x, y, z)) {
        return sigNumEntry(label, self(c), self(x), self(y), self(z));
    } else if (isSigVBargraph(sig, label, x, y, z)) {
        return sigVBargraph(label, self(x), self(y), self(z));
    } else if (isSigHBargraph(sig, label, x, y, z)) {
        return sigHBargraph(label, self(x), self(y), self(z));
    }

    // Sounfile length, rate, channels, buffer
    else if (isSigSoundfile(sig, label)) {
        return sig;
    } else if (isSigSoundfileLength(sig, sf)) {
        return sigSoundfileLength(self(sf));
    } else if (isSigSoundfileRate(sig, sf)) {
        return sigSoundfileRate(self(sf));
    } else if (isSigSoundfileChannels(sig, sf)) {
        return sigSoundfileChannels(self(sf));
    } else if (isSigSoundfileBuffer(sig, sf, x, y)) {
        return sigSoundfileBuffer(self(sf), self(x), self(y));
    }

    // Attach, Enable, Control
    else if (isSigAttach(sig, x, y)) {
        return sigAttach(self(x), self(y));
    } else if (isSigEnable(sig, x, y)) {
        return sigEnable(self(x), self(y));
    } else if (isSigControl(sig, x, y)) {
        return sigControl(self(x), self(y));
    }

    else {
        stringstream error;
        error << "ERROR : unrecognized signal : " << *sig << endl;
        throw faustexception(error.str());
    }
    return 0;
}

#endif