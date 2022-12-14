/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2019 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::odeFiniteRate

Description
    An odeFiniteRate solver for chemistry

SourceFiles
    odeFiniteRate.C

\*---------------------------------------------------------------------------*/

#ifndef odeFiniteRate_H
#define odeFiniteRate_H

#include "FiniteRateChemistrySolver.H"
#include "ODESolver.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                            Class ode Declaration
\*---------------------------------------------------------------------------*/

template<class ChemistryModel>
class odeFiniteRate
:
public FiniteRateChemistrySolver<ChemistryModel>
{
  // Private Data

  dictionary coeffsDict_;

  mutable autoPtr<ODESolver> odeSolver_;

  // Solver data
  mutable scalarField cTp_;


 public:

  //- Runtime type information
  TypeName("odeFiniteRate");


  // Constructors

  //- Construct from thermo
  odeFiniteRate(typename ChemistryModel::reactionThermo& thermo);


  //- Destructor
  virtual ~odeFiniteRate();


  // Member Functions

  //- Update the concentrations and return the chemical time
  virtual void solve
  (
      scalarField& c,
      scalar& T,
      scalar& p,
      scalar& deltaT,
      scalar& subDeltaT
  ) const;

  //- Update the concentrations and return the chemical time, do not update temperature (PATO)
  virtual void solveFixedT
  (
      scalarField &c,
      scalar& T,
      scalar& p,
      scalar& deltaT,
      scalar& subDeltaT
  ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#include "odeFiniteRate.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
