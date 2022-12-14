/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
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
    along with OpenFOAM.  If SpeciesConservationt, see <http://www.gnu.org/licenses/>.

Class
    Foam::SpeciesConservationMaterialChemistryModel

Description

SourceFiles
    SpeciesConservationMaterialChemistryModel.H
    SpeciesConservationMaterialChemistryModel.C

\*---------------------------------------------------------------------------*/

#ifndef SpeciesConservationMaterialChemistryModel_H
#define SpeciesConservationMaterialChemistryModel_H

#include "simpleMaterialChemistryModel.H"
#include "mathFunctions.H"
#include "VolumeAblationFvPatchScalarField.H"
#include "speciesBCFvPatchScalarField.H"
#include "coupledMixedFvPatchScalarField.H"
#include "IOFunctions.H"
#include "simpleMaterialPropertiesModel.H"
#include "simpleGasPropertiesModel.H"
#include "simpleMassModel.H"
#include "simplePyrolysisModel.H"
#include "simpleVolumeAblationModel.H"
#include "psiReactionThermo.H"
#include "BasicFiniteRateChemistryModel.H"
#include "OFstream.H"
#include "thermoPhysicsTypes.H"
#include "basicMultiComponentMixture.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class SpeciesConservationMaterialChemistryModel Declaration
\*---------------------------------------------------------------------------*/

class SpeciesConservationMaterialChemistryModel
:
public simpleMaterialChemistryModel
{

 protected:

  //- Mutation++ Mixture name
  word mixtureMutation;

  //- Mutation++ Mixture Options
  autoPtr<Mutation::MixtureOptions> optEq_;

  //- Mutation++ Mixture
  autoPtr<Mutation::Mixture>& mix_;

  //- Number of species in the mixture
  int ns_mix;

  //- Index of the C(gr) for in-depth oxidation of the solid
  int iCs_;

  //- Heterogeneous rate [kg/m3/s]
  volScalarField& omegaHeterogeneousRate_;

  //- Heterogeneous flux [W/m3]
  volScalarField& omegaHeterogeneousEnergy_;

  //- Material properties directory
  const fileName materialPropertiesDirectory;

  //- ConstantProperties dictionary
  IOdictionary constantPropertiesDictionary;

  //- List of the element names
  wordList& elementNames_;

  //- List of the species names
  wordList& speciesNames_;

  //- Species mass fractions
  PtrList<volScalarField>& massFractions_;

  //- Species mass fractions from previous time step
  PtrList<volScalarField>& oldMassFractions_;

  //- Species mole fractions
  PtrList<volScalarField>& moleFractions_;

  //- Default species mass/mole fractions
  volScalarField Ydefault_;

  //- Tortuosity
  dimensionedScalar eta0_;

  //- Volume Ablation patches - BoundaryConditions Model
  labelList volumeAblationPatches_;

  //- Volume Ablation BC for the SMEB patches
  List<VolumeAblationBoundaryConditions*> volumeAblationBC_;

  //- Volume Ablation BC for the SMEB patches
  List<speciesBCBoundaryConditions*> speciesBC_;

  //- Temperature (K)
  volScalarField& T_;

  //- Pressure (Pa)
  const volScalarField& p_;

  //- Wall enthalpy using the environment composition [J/kg]
  volScalarField& h_ew_;

  //- Surface Interpolation Scheme table
  multivariateSurfaceInterpolationScheme<scalar>::fieldTable fields;

  //- Psi reaction thermo model
  autoPtr<psiReactionThermo> pThermo_;

  //- Psi reaction thermo model reference
  psiReactionThermo& thermo_;

  //- Chemistry model coupled to psithermo model
  autoPtr<BasicFiniteRateChemistryModel<psiReactionThermo>> pChemistry_;

  //- Referemce of the chemistry model coupled to psithermo model
  BasicFiniteRateChemistryModel<psiReactionThermo>& chemistry_;

  //- Latest estimation of MaterialChemistry integration
  scalar dtChem_;

  //- CHEMKIN composition
  basicMultiComponentMixture& composition_;

  //- CHEMKIN species mass fractions
  PtrList<volScalarField>& Y_;

  //- Index species in Mutation++
  labelList& speciesIndexMutation_;

  //- Init flag for mixture
  Switch init_;

  //- Pyrolysis model
  simplePyrolysisModel& pyrolysisModel_;

  //- Pyrolysis elemental reaction rates [kg/m3/s]
  const PtrList<volScalarField>& pi_;

  //- Material Properties model
  simpleMaterialPropertiesModel& materialPropertiesModel_;

  //- Charred enthalpy [J/kg]
  volScalarField& h_c_;

  //- Gas Properties model
  simpleGasPropertiesModel& gasPropertiesModel_;

  //- Gas density [kg/m3]
  volScalarField& rho_g_;

  //- Gas volume fraction [-]
  volScalarField& eps_g_;

  //- Species diffusion coefficient
  PtrList<volScalarField>& Dm_;

  //- Mass model
  simpleMassModel& massModel_;

  //- Pyrolysis gas mass flow rate [kg/m3/s]
  const volVectorField& mDotG_;

  //- Pyrolysis gas mass flux flow rate interpolated on the faces [kg/m3/s] - updated in Mass
  const surfaceVectorField& mDotGFace_;

  //- Pyrolysis gas mass flow rate at the walls [kg/m2/s] - updated in BprimeBoundaryConditions
  const volScalarField& mDotGw_;

  //- Volume ablation model
  simpleVolumeAblationModel& volumeAblationModel_;

  //- Volume ablation type
  const word volumeAblationType_;

  //- Gas properties type
  const word gasPropertiesType_;

  //- Diffusive part []
  PtrList<volScalarField> diffY_;

 public:

  //- Runtime type information
  TypeName("SpeciesConservation");

  // Constructors

  //- Construct from mesh and phase name
  SpeciesConservationMaterialChemistryModel(const fvMesh& mesh,const word& dictName);

  //- Destructor
  virtual ~SpeciesConservationMaterialChemistryModel();

  // Member Functions

  //- Update the species conservation equations
  virtual void update();

  //- Initialize the mixture
  inline Switch init();

  //- Update the species boundary conditions
  inline void updateYBC();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
