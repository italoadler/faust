
#include "sigTransform.hh"
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

//------------------------------------------------------------------------------
// Recursive transformation
//------------------------------------------------------------------------------

Tree Transform::self(Tree t)
{
    Tree r;
    if (!fResult.get(t, r)) {
        r = transformation(t);
        fResult.set(t, r);
    }
    return r;
}

Tree Transform::mapself(Tree lt)
{
    if (isNil(lt)) {
        return lt;
    } else {
        return cons(self(hd(lt)), mapself(tl(lt)));
    }
}

Tree SignalIdentity::transformation(Tree sig)
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
        return sigFixDelay(self(x), self(y));
    } else if (isSigPrefix(sig, x, y)) {
        return sigPrefix(self(x), self(y));
    } else if (isSigIota(sig, x)) {
        return sigIota(self(x));
    } else if (isSigBinOp(sig, &i, x, y)) {
        return sigBinOp(i, self(x), self(y));
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
        return sigIntCast(self(x));
    } else if (isSigFloatCast(sig, x)) {
        return sigFloatCast(self(x));
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