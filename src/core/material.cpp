#include "material.hpp"

#include <algorithm>

namespace mocc{
    Material::Material(VecF xsab, VecF xsnf, VecF xskf, VecF xsch,
                       std::vector<VecF> scat):
        xsab_(xsab.size()),
        xstr_(xsab.size()),
        xsnf_(xsab.size()),
        xskf_(xsab.size()),
        xsch_(xsab.size()),
        xssc_(scat)
    {
        assert(xsab.size() == xsnf.size());
        assert(xsab.size() == xskf.size());
        assert(xsab.size() == xsch.size());

        for( unsigned ig=0; ig<xsab.size(); ig++ ) {
            xsab_(ig) = xsab[ig];
            xsnf_(ig) = xsnf[ig];
            xskf_(ig) = xskf[ig];
            xsch_(ig) = xsch[ig];
        }

        // Normalize chi. Wow. Very precision. Much digits.
        if( std::any_of(xsch_.begin(), xsch_.end(),
                    [](real_t v){return v>0.0;}) ) {
            real_t chi_sum = 0.0;
            for( auto c: xsch_ ) {
                chi_sum += c;
            }
            for( auto &c: xsch_ ) {
                c /= chi_sum;
            }
        }

        int ng = xsab_.size();
        // Simple calculation of transport cross section
        for( int ig=0; ig<ng; ig++ ) {
            xstr_(ig) = xsab[ig] + xssc_.out(ig);
        }
    }
};

