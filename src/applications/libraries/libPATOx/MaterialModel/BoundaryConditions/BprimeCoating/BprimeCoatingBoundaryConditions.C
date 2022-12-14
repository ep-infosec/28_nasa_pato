/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2015 OpenFOAM Foundation
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
    along with OpenFOAM.  If Bprimet, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "BprimeCoatingBoundaryConditions.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::BprimeCoatingBoundaryConditions::BprimeCoatingBoundaryConditions
(
    const fvMesh& mesh,
    const word& phaseName,
    const word& dictName,
    const label& currentPatchID,
    const dictionary dict
)
  :
mesh_(mesh),
phaseName_(phaseName),
dictName_(dictName),
currentPatchID_(currentPatchID),
dict_(dict),
dynamicMesh_(isA<dynamicFvMesh>(mesh)),
nameBprimeFile_(dict_.lookup("BprimeFile")),
mDotGw_(meshLookupOrConstructScalar(mesh, "mDotGw",dimensionedScalar("0", dimMass/pow(dimLength,2)/dimTime, scalar(0.0)))),
mDotCw_(meshLookupOrConstructScalar(mesh, "mDotCw", dimensionedScalar("0", dimMass/pow(dimLength,2)/dimTime, scalar(0.0)))),
BprimeCw_(meshLookupOrConstructScalar(mesh, "BprimeCw",dimensionedScalar("0", dimless, scalar(0.0)))),
BprimeGw_(meshLookupOrConstructScalar(mesh, "BprimeGw",dimensionedScalar("0", dimless, scalar(0.0)))),
qRadEmission_(meshLookupOrConstructScalar(mesh, "qRadEmission",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
qRadAbsorption_(meshLookupOrConstructScalar(mesh, "qRadAbsorption",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
qAdvPyro_(meshLookupOrConstructScalar(mesh, "qAdvPyro",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
qAdvChar_(meshLookupOrConstructScalar(mesh, "qAdvChar",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
qConv_(meshLookupOrConstructScalar(mesh, "qConv",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
qCond_(meshLookupOrConstructScalar(mesh, "qCond",dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
recession_(meshLookupOrConstructScalar(mesh, "recession", dimensionedScalar("0", dimLength, scalar(0.0)))),
rhoeUeCH(meshLookupOrConstructScalar(mesh, "rhoeUeCH", dimensionedScalar("0", dimMass/ pow3(dimLength)/ dimTime, scalar(0.0)))),
blowingCorrection(meshLookupOrConstructScalar(mesh, "blowingCorrection",dimensionedScalar("1", dimless, scalar(1.0)))),
h_w(meshLookupOrConstructScalar(mesh, "h_w",dimensionedScalar("0", pow(dimLength,2)/pow(dimTime,2), scalar(0.0)))),
h_r(meshLookupOrConstructScalar(mesh, "h_r",dimensionedScalar("0", pow(dimLength,2)/pow(dimTime,2), scalar(0.0)))),
Tbackground(meshLookupOrConstructScalar(mesh, "Tbackground", dimensionedScalar("0", dimTemperature, scalar(0.0)))),
lambda(meshLookupOrConstructScalar(mesh, "lambda", dimensionedScalar("0", dimless, scalar(0.0)))),
qRad(meshLookupOrConstructScalar(mesh, "qRad", dimensionedScalar("0", dimMass/ pow3(dimTime)/ dimLength, scalar(0.0)))),
heatOn(meshLookupOrConstructScalar(mesh, "heatOn", dimensionedScalar("0", dimless, scalar(0.0)))),
//update in Mass Model
mDotG_(meshLookupOrConstructVector(mesh, "mDotG", dimensionedVector("zero", dimMass/(dimLength*dimLength*dimTime), vector(0, 0, 0)))),
mDotGFace_(meshLookupOrConstructSurfaceVector(mesh, "mDotGface", linearInterpolate(mDotG_))),
//update in Gas Properties Model
h_g_(meshLookupOrConstructScalar(mesh, "h_g", dimensionedScalar("0", dimensionSet(0, 2, -2, 0, 0, 0, 0), 0.0))),
//update in Material Properties Model
h_c_(meshLookupOrConstructScalar(mesh, "h_c", dimensionedScalar("0", dimensionSet(0, 2, -2, 0, 0, 0, 0), 0.0))),
emissivity_(meshLookupOrConstructScalar(mesh, "emissivity", dimensionedScalar("0", dimless, scalar(0)))),
absorptivity_(meshLookupOrConstructScalar(mesh, "absorptivity", dimensionedScalar("0", dimless, scalar(0)))),
k_(meshLookupOrConstructTensor(mesh, "k", dimensionedTensor("0", dimensionSet(1, 1, -3, -1, 0, 0, 0), tensor(1,0,0,0,1,0,0,0,1)))),
// MUST_READ fields
rho_s_(meshLookupOrConstructScalar(mesh, "rho_s")),
p_(meshLookupOrConstructScalar(mesh, "p")),
T_(meshLookupOrConstructScalar(mesh, "Ta")),
materialDict_(
    IOobject
    (
        IOobject::groupName(dictName+"Properties", phaseName),
        mesh.time().constant(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::NO_WRITE,
        false
    )
),
materialPropertiesDirectory
(
    fileName(materialDict_.subDict("MaterialProperties").lookup("MaterialPropertiesDirectory")).expand()
),
constantPropertiesDictionary
(
    IOobject
    (
        "constantProperties",
        materialPropertiesDirectory,
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::NO_WRITE
    )
),
sigmaSB(::constant::physicoChemical::sigma),
initialPosition_
(
    meshLookupOrConstructVector(mesh,  "initialPosition",    dimensionedVector("0", dimLength, vector(0.0,0.0,0.0)))
),
debug_(materialDict_.lookupOrDefault<Switch>("debug","no")),
neededFields_(neededFields()),
boundaryMapping_
(
    simpleBoundaryMappingModel::New(
        mesh_,
        neededFields_,
        dict_
    )
),
boundaryMapping_ptr(&boundaryMapping_()),
coatingRecession_(meshLookupOrConstructScalar(mesh, "coatingRecession", dimensionedScalar("0", dimLength, scalar(0.0)))),
coatingHeight_(readScalar(dict_.lookup("coatingHeight"))),
coatingTableFileName_(dict_.lookup("coatingTableFileName")),
coatingBprimeFileName_(dict_.lookup("coatingBprimeFileName")),
ne_Bprime(0),
ns_Bprime(0),
recessionFlag(dict_.lookupOrDefault<Switch>("recessionFlag","yes"))
{
  initBprimeMixture();
  coatingTable_=readFileData(coatingTableFileName_);
  if (coatingTable_.size()!=5) { // The columns must be: T h_c eps alpha rho_s
    FatalErrorInFunction << coatingTableFileName_ << " must have 5 columns." << exit(FatalError);
  }
  if(dynamicMesh_) {

    cellMotionU_ptr =
        new volVectorField
    (
        IOobject
        (
            "cellMotionU",
            mesh.time().timeName(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        mesh
    );

    forAll(initialPosition_.boundaryField()[currentPatchID_], faceI) {
      initialPosition_.boundaryFieldRef()[currentPatchID_][faceI] =  mesh_.Cf().boundaryField()[currentPatchID_][faceI];
    }
  }
  typeNameMaterialPropertiesModel = "no";
  if (this->materialDict_.isDict("MaterialProperties")) {
    typeNameMaterialPropertiesModel = this->materialDict_.subDict("MaterialProperties").template lookupOrDefault<word>("MaterialPropertiesType","no");
  }
  if (typeNameMaterialPropertiesModel=="GradedPorous") {
    Info << "coatingTableFileName is not used at the boundary because the MaterialPropertiesType=GradedPorous" << endl;
  }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //


Foam::BprimeCoatingBoundaryConditions::~BprimeCoatingBoundaryConditions()
{
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::BprimeCoatingBoundaryConditions::initBprimeMixture()
{
  RectangularMatrix<scalar> table = readBprimeTable(nameBprimeFile_);
  BprimeT_ptr = new BprimeTable(table);
  RectangularMatrix<scalar> table2 = readBprimeTable(coatingBprimeFileName_);
  coatingBprimeT_ptr = new BprimeTable(table2);
}

void Foam::BprimeCoatingBoundaryConditions::update()
{
  updateCoatingBC();

  if (debug_) {
    Info << "--- update BoundaryMapping --- Foam::BprimeCoatingBoundaryConditions::update()" << endl;
  }
  updateBoundaryMapping();

  if (debug_) {
    Info << "--- update Bprime --- Foam::BprimeCoatingBoundaryConditions::update()" << endl;
  }
  updateBprimeBC();

  if (debug_) {
    Info << "--- update temperature --- Foam::BprimeCoatingBoundaryConditions::update()" << endl;
  }
  updateTemperatureBC();

  if(dynamicMesh_) {
    if (debug_) {
      Info << "--- update motion --- Foam::BprimeCoatingBoundaryConditions::update()" << endl;
    }
    updateMotionBC();

    if(this->debug_) {
      Info << "--- end --- Foam::BprimeCoatingBoundaryConditions::update()" << endl;
    }
  }
}

void Foam::BprimeCoatingBoundaryConditions::updateCoatingBC()
{
  forAll(mesh_.boundaryMesh()[currentPatchID_], faceI) {
    if (coatingRecession_.boundaryField()[currentPatchID_][faceI]<=coatingHeight_) {
      if (typeNameMaterialPropertiesModel!="GradedPorous") {
        scalar& h_c_BF = h_c_.boundaryFieldRef()[currentPatchID_][faceI];
        scalar& emissivity_BF = emissivity_.boundaryFieldRef()[currentPatchID_][faceI];
        scalar& absorptivity_BF = absorptivity_.boundaryFieldRef()[currentPatchID_][faceI];
        scalar& rho_s_bf = rho_s_.boundaryFieldRef()[currentPatchID_][faceI];
        const scalar& T_BF = T_.boundaryField()[currentPatchID_][faceI];
        h_c_BF = linearInterpolation(coatingTable_[0], coatingTable_[1], T_BF);
        emissivity_BF = linearInterpolation(coatingTable_[0], coatingTable_[2], T_BF);
        absorptivity_BF = linearInterpolation(coatingTable_[0], coatingTable_[3], T_BF);
        rho_s_bf = linearInterpolation(coatingTable_[0], coatingTable_[4], T_BF);
      }
    }
  }
}

void Foam::BprimeCoatingBoundaryConditions::updateBoundaryMapping()
{
  // Update all the other fields (different than Ta) needed for BprimeCoatingBoundaryConditions (p, rhoeUeCH, ...)
  forAll(boundaryMapping_ptr->mappingFieldsName(), fieldI) {
    if (!isA<boundaryMappingFvPatchScalarField>(const_cast<volScalarField&>(mesh_.objectRegistry::lookupObject<volScalarField>(boundaryMapping_ptr->mappingFieldsName()[fieldI])).boundaryField()[currentPatchID_])) {
      boundaryMapping_ptr->update(mesh_.time().value(),currentPatchID_,boundaryMapping_ptr->mappingFieldsName()[fieldI]);
    } else { // correct the fields with a different mappingBoundary patch type
      const_cast<volScalarField&>(mesh_.objectRegistry::lookupObject<volScalarField>(boundaryMapping_ptr->mappingFieldsName()[fieldI])).correctBoundaryConditions();
    }
  }
}

void Foam::BprimeCoatingBoundaryConditions::updateMotionBC()
{
  // OpenFOAM 4.x
  if(mesh_.objectRegistry::foundObject<volVectorField>("cellMotionU")) {
    volVectorField& cellMotionU_ = const_cast<volVectorField&>(meshLookupOrConstructVector(mesh_, "cellMotionU"));
    forAll(cellMotionU_.boundaryFieldRef()[currentPatchID_], faceI) {
      // not used yet
      scalar failureFraction=0;
      scalar erosionRate=0;

      // Already updated
      const scalar& rhoeUeCH_BF = rhoeUeCH.boundaryField()[currentPatchID_][faceI];
      const vector nf =
          - mesh_.Sf().boundaryField()[currentPatchID_][faceI]
          / mesh_.magSf().boundaryField()[currentPatchID_][faceI];
      const scalar& blowingCorrection_BF = blowingCorrection.boundaryField()[currentPatchID_][faceI];
      const scalar& BprimeCw_BF = BprimeCw_.boundaryField()[currentPatchID_][faceI];
      const scalar& rho_s_bf = rho_s_.boundaryField()[currentPatchID_][faceI];

      // fluxFactorThreshold (2D axi boundary mapping): Mesh motion direction is projected on the direction of pointMotionDirection when the value of the fluxFactor is higher than the set Threshold, otherwise motion is applied directly
      // along the face normal. This is useful to preserve the topology for large and strongly distorted deformations.
      vector normal_ = nf;
      if (isA<boundaryMappingFvPatchScalarField>(rhoeUeCH.boundaryField()[currentPatchID_])) {
        boundaryMappingFvPatchScalarField& rhoeUeCH_bm=refCast<boundaryMappingFvPatchScalarField>(rhoeUeCH.boundaryFieldRef()[currentPatchID_]);
        autoPtr<simpleBoundaryMappingModel>& boundaryMapping_rhoeUeCH = rhoeUeCH_bm.boundaryMapping_;
        if (boundaryMapping_rhoeUeCH->fluxFactor_ptr) {
          const scalar& fluxThreshold_ = boundaryMapping_rhoeUeCH->fluxFactor_ptr->fluxFactorThreshold().value();
          const scalar& fluxMap_ = boundaryMapping_rhoeUeCH->fluxFactor_ptr->fluxMap().boundaryField()[currentPatchID_][faceI];
          const vector& pointMotionDirection_ = boundaryMapping_rhoeUeCH->fluxFactor_ptr->pointMotionDirection();
          const Switch& moreThanTresholdFlag_ = boundaryMapping_rhoeUeCH->fluxFactor_ptr->moreThanThresholdFlag();
          if (moreThanTresholdFlag_) {
            if (fluxMap_ > fluxThreshold_) {
              normal_ = (1 / (nf & pointMotionDirection_)) * pointMotionDirection_;
            }
          } else {
            if (fluxMap_ < fluxThreshold_) {
              normal_ = (1 / (nf & pointMotionDirection_)) * pointMotionDirection_;
            }
          }
        }
      }

      // Will be updated
      vector&  cellMotionU_BF = cellMotionU_.boundaryFieldRef()[currentPatchID_][faceI];

      if (coatingRecession_.boundaryField()[currentPatchID_][faceI]<=coatingHeight_) {
        cellMotionU_BF=vector(0,0,0);
        coatingRecession_.boundaryFieldRef()[currentPatchID_][faceI]+=mesh_.time().deltaT().value()
            *((BprimeCw_BF* rhoeUeCH_BF * blowingCorrection_BF * (1 + failureFraction)
               + erosionRate)* 1 / rho_s_bf);
      } else {
        cellMotionU_BF=
            (BprimeCw_BF* rhoeUeCH_BF * blowingCorrection_BF * (1 + failureFraction)
             + erosionRate)
            * 1 / rho_s_bf * normal_;

        if (!recessionFlag) {
          cellMotionU_BF = vector(0,0,0);
        }
      }
    }
  }

  forAll(recession_.boundaryField()[currentPatchID_], faceI) {
    recession_.boundaryFieldRef()[currentPatchID_][faceI] = mag(initialPosition_.boundaryField()[currentPatchID_][faceI] -  mesh_.Cf().boundaryField()[currentPatchID_][faceI]);
  }
}

void Foam::BprimeCoatingBoundaryConditions::updateTemperatureBC()
{
  forAll(T_.boundaryFieldRef()[currentPatchID_], faceI) {
    // not used yet
    scalar wallSpeciesDiffusion = 1;

    // Updated by BoundaryMapping
    scalar& h_r_BF = h_r.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& Tbackground_BF = Tbackground.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qRad_BF = qRad.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& heatOn_BF = heatOn.boundaryFieldRef()[currentPatchID_][faceI];

    // Already updated
    const scalar& rhoeUeCH_BF = rhoeUeCH.boundaryFieldRef()[currentPatchID_][faceI];
    const vector nf =
        - mesh_.Sf().boundaryField()[currentPatchID_][faceI]
        / mesh_.magSf().boundaryField()[currentPatchID_][faceI];
    const scalar invDx_BF = mesh_.deltaCoeffs().boundaryField()[currentPatchID_][faceI];
    const tmp<scalarField> Tint_tmp = T_.boundaryField()[currentPatchID_].patchInternalField();
    const scalarField& Tint_ = Tint_tmp();
    const scalar Tint_BF = Tint_[faceI];
    const scalar mDotGFace_BF = mDotGFace_.boundaryField()[currentPatchID_][faceI]&(-nf);
    const scalar h_g_BF = h_g_.boundaryField()[currentPatchID_][faceI];
    const scalar h_c_BF = h_c_.boundaryField()[currentPatchID_][faceI];
    const scalar emissivity_BF = emissivity_.boundaryField()[currentPatchID_][faceI];
    const scalar absorptivity_BF = absorptivity_.boundaryField()[currentPatchID_][faceI];
    const tensor k_BF = k_.boundaryField()[currentPatchID_][faceI];
    const scalar kProj_BF = nf & k_BF & nf; // Projection of k on the surface normal
    const scalar& blowingCorrection_BF = blowingCorrection.boundaryFieldRef()[currentPatchID_][faceI];
    const scalar& BprimeCw_BF = BprimeCw_.boundaryFieldRef()[currentPatchID_][faceI];
    const scalar& h_w_BF = h_w.boundaryFieldRef()[currentPatchID_][faceI];

    // Will be updated
    scalar& qRadEmission_BF = qRadEmission_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qRadAbsorption_BF = qRadAbsorption_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qAdvPyro_BF = qAdvPyro_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qAdvChar_BF = qAdvChar_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qConv_BF = qConv_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& qCond_BF = qCond_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& T_BF = T_.boundaryFieldRef()[currentPatchID_][faceI];

    // Surface energy balance: heat fluxes and temperature
    qRadEmission_BF = - emissivity_BF * sigmaSB.value() * (pow4(T_BF) - pow4(Tbackground_BF));
    qRadAbsorption_BF = absorptivity_BF * qRad_BF;

    if(heatOn_BF == 1) {
      qAdvPyro_BF = mDotGFace_BF * (h_g_BF - h_w_BF);
      qAdvChar_BF =  BprimeCw_BF * rhoeUeCH_BF * blowingCorrection_BF * (h_c_BF - h_w_BF);
      qConv_BF = rhoeUeCH_BF * blowingCorrection_BF * (h_r_BF - h_w_BF * wallSpeciesDiffusion) ;
      T_BF =
          Tint_BF
          + (
              1. / (kProj_BF * invDx_BF) *
              (
                  qConv_BF + qAdvPyro_BF + qAdvChar_BF + qRadEmission_BF + qRadAbsorption_BF
              )
          );
    } else {
      qAdvChar_BF = 0;
      qConv_BF = 0;
      qAdvPyro_BF = 0;
      T_BF =
          Tint_BF
          + (
              1. / (kProj_BF * invDx_BF) *
              (
                  qRadEmission_BF + qRadAbsorption_BF
              )
          );
    }


    qCond_BF = (kProj_BF * invDx_BF)* (T_BF - Tint_BF);
  }



}

void Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()
{
  if (debug_) {
    Info << "--- currentPatchID_=" << currentPatchID_ << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()"  << endl;
    Info << "--- update Boundary Mapping for patch " << mesh_.boundaryMesh()[currentPatchID_].name() << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()"  << endl;
  }

  forAll(BprimeCw_.boundaryField()[currentPatchID_], faceI) {

    // Species mole fraction at the wall
    scalarList p_Xw_(ns_Bprime);
    // Elements mass fraction inside the material
    scalarList p_Ykg_(ne_Bprime);
    // Elements mass fraction in the environment
    scalarList p_Yke_(ne_Bprime);

    if (debug_ && faceI==0) {
      Info << "--- updated by Boundary Mapping --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }
    // Updated by Boundary Mapping
    scalar& rhoeUeCH_BF = rhoeUeCH.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& lambda_BF = lambda.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& heatOn_BF = heatOn.boundaryFieldRef()[currentPatchID_][faceI];

    if (debug_ && faceI==0) {
      Info << "--- already updated --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }
    // Already updated
    const vector nf =
        - mesh_.Sf().boundaryField()[currentPatchID_][faceI]
        / mesh_.magSf().boundaryField()[currentPatchID_][faceI];
    const scalar mDotGFace_BF = mDotGFace_.boundaryField()[currentPatchID_][faceI]&(-nf);
    const scalar p_BF = p_.boundaryField()[currentPatchID_][faceI];
    const scalar T_BF = T_.boundaryField()[currentPatchID_][faceI];

    if (debug_ && faceI==0) {
      Info << "--- will be updated --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }
    // Will be updated
    scalar& mDotGw_BF = mDotGw_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& mDotCw_BF = mDotCw_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& blowingCorrection_BF = blowingCorrection.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& BprimeGw_BF = BprimeGw_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& BprimeCw_BF = BprimeCw_.boundaryFieldRef()[currentPatchID_][faceI];
    scalar& h_w_BF = h_w.boundaryFieldRef()[currentPatchID_][faceI];

    if (debug_ && faceI==0) {
      Info << "--- update pyrolysis gas flow rate --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }
    // pyrolysis gas flow rate
    mDotGw_BF = mDotGFace_BF;

    if (debug_ && faceI==0) {
      Info << "--- blowing condition --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
      Info << "--- heatOn_BF=" << heatOn_BF << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()"<<endl;
      Info << "--- rhoeUeCH_BF" << rhoeUeCH_BF << endl;
    }
    if(heatOn_BF == 1) {
      // Blowing correction
      bool condition_ = (rhoeUeCH_BF <= 1e-4);
      if (materialDict_.isDict("Pyrolysis")) {
        word typePyro = materialDict_.subDict("Pyrolysis").lookupOrDefault<word>("PyrolysisType", "noPyrolysisSolver<specifiedPyrolysisModel>");
        typePyro.replaceAll("PyrolysisSolver<specifiedPyrolysisModel>", "");
        if (typePyro != "no") {
          condition_ = (mDotGFace_BF <= 1e-6) || (rhoeUeCH_BF <= 1e-4);
        }
      }
      if (debug_ && faceI==0) {
        Info << "--- update blowing correction --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
        Info << "--- rhoeUeCH=" << rhoeUeCH_BF << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
        Info << "--- BprimeCw_BF=" << BprimeCw_BF << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
        Info << "--- mDotGFace_BF=" << mDotGFace_BF << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
        Info << "--- condition_=" << condition_ << " --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
      }

      if (rhoeUeCH_BF == 0) {
        FatalErrorInFunction << "rhoeUeCH_BF == 0 for rhoeUeCH\n" << rhoeUeCH << exit(FatalError);
      }
      if ((mDotGFace_BF / (rhoeUeCH_BF * blowingCorrection_BF) + BprimeCw_BF )==0) {
        condition_=1;
      }

//          bool condition_ = (BprimeCw_BF == 0 || mDotGFace_BF <= 1e-6) || (rhoeUeCH_BF <= 1e-4);
      if (condition_) {
        blowingCorrection_BF= 1.0;
      } else {
        blowingCorrection_BF =
            ::log
            (
                1. + 2. * lambda_BF *
                (
                    mDotGFace_BF / (rhoeUeCH_BF * blowingCorrection_BF)
                    + BprimeCw_BF
                )
            ) /
            (
                2. * lambda_BF *
                (
                    mDotGFace_BF / (rhoeUeCH_BF * blowingCorrection_BF)
                    + BprimeCw_BF
                )
            );
      }

      if (debug_ && faceI==0) {
        Info << "--- update BprimeG --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
      }
      // BprimeG
      BprimeGw_BF = max(mDotGFace_BF / (rhoeUeCH_BF * blowingCorrection_BF),0);
    }

    if(heatOn_BF == 0) {
      if (debug_ && faceI==0) {
        Info << "--- update (BprimeC, mDotC, hw) == 0 --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
      }
      BprimeCw_BF = 0;
      mDotCw_BF = 0;
      h_w_BF = 0;
      continue;
    }

    if (debug_ && faceI==0) {
      Info << "--- update BprimeC and hw --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }

    forAll(p_Yke_, elementI) {
      p_Yke_[elementI] = Yke_ref[elementI];
    }

    forAll(p_Ykg_, elementI) {
      volScalarField& Ykgw = const_cast<volScalarField&>(mesh_.objectRegistry::lookupObject<volScalarField>(Ykg_names[elementI]));
      p_Ykg_[elementI] = Ykgw.boundaryFieldRef()[currentPatchID_][faceI];
    }

    // BprimeC and h_w
    computeSurfaceMassBalance(faceI, p_Yke_, p_Ykg_, p_BF, T_BF, BprimeGw_BF, BprimeCw_BF, h_w_BF, p_Xw_);

    forAll(p_Xw_, speciesI) {
      volScalarField& Xw = const_cast<volScalarField&>(mesh_.objectRegistry::lookupObject<volScalarField>(Xw_names[speciesI]));
      Xw.boundaryFieldRef()[currentPatchID_][faceI] = p_Xw_[speciesI];
    }

    if (debug_ && faceI==0) {
      Info << "--- update char ablation rate --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
    }
    // char ablation rate
    mDotCw_BF = BprimeCw_BF* rhoeUeCH_BF * blowingCorrection_BF;

    if (!recessionFlag) {
      BprimeCw_BF = 0;
      mDotCw_BF = 0;
    }

    if(!dynamicMesh_) {
      if (debug_ && faceI==0) {
        Info << "--- update (BprimeC, mDotC) == 0 --- Foam::BprimeCoatingBoundaryConditions::updateBprimeBC()" << endl;
      }
      BprimeCw_BF = 0;
      mDotCw_BF = 0;
    }
  }
}

Foam::wordList Foam::BprimeCoatingBoundaryConditions::neededFields()
{
  wordList neededFields;
  neededFields.append("p");
  neededFields.append("h_r");
  neededFields.append("Tbackground");
  neededFields.append("lambda");
  neededFields.append("rhoeUeCH");
  neededFields.append("qRad");
  neededFields.append("heatOn");
  return neededFields;
}

RectangularMatrix<scalar> Foam::BprimeCoatingBoundaryConditions::readBprimeTable(fileName BprimeFile)
{
  // read and store the B' table into the RAM for faster access
  Info << "Reading the B' Table" << nl;
  fileName BprimeFileName(changeEnviVar(BprimeFile));
  if(debug_) {
    Info << "--- BprimeFileName=" << BprimeFileName << endl;
  }
//  fileName BprimeFileName(fileName(nameBprimeFile_).expand());
  IFstream BprimeInputFile(BprimeFileName.c_str());  // opens an input file

  if (BprimeInputFile.good() == false) { // checks the input file is opened
    FatalErrorInFunction << "Bprime table not found. Either update the path or change the simulation options."
                         << exit(FatalError); // exit
  }

  IFstream BprimeInputFileTemp(BprimeFileName.c_str());  // open a second input file
  Info << "The Bprime table must have 5 columns: p, B'g, B'c, T, Hg, ..." << nl;

  int columnTableBp = 5;
  int rawTableBp = 0;
  scalar tempBp, rawTableFracBp, rawTableIntBp;
  int i_rawBp = 0;

  while (true) {
    BprimeInputFileTemp >> tempBp;
    if (BprimeInputFileTemp.eof() == 1)
      break;
    i_rawBp++;
  }
  rawTableFracBp =
      modf
      (
          static_cast<scalar>(i_rawBp) /
          static_cast<scalar>(columnTableBp),
          &rawTableIntBp
      );

  if (rawTableFracBp != 0) {
    FatalErrorInFunction << "... and it does not seem to have 5 columns." << nl
                         << rawTableIntBp << " lines and "
                         << rawTableFracBp * columnTableBp << " 'scalar' have been read."
                         << exit(FatalError);
  } else {
    rawTableBp = i_rawBp / columnTableBp;
    Info << "The Bprime table has been read (" << rawTableBp << " lines)."
         << nl;
  }

  RectangularMatrix<scalar> Table(rawTableBp, columnTableBp, 0);

  for (int x = 0; x < rawTableBp; x++) {
    for (int i = 0; i < columnTableBp; i++) {
      BprimeInputFile >> Table[x][i];
    }
  }

  return Table;
  // BprimeT table is an object of the class BprimeSI, which handles the B' table interpolations when called
//  BprimeT_ptr = new BprimeTable(Table);

//  Info << "The B' Table has been read" << nl;
}


void Foam::BprimeCoatingBoundaryConditions::computeSurfaceMassBalance(label faceI, scalarList Yke_, scalarList Ykg_, scalar pw_, scalar Tw_, scalar Bg_, scalar& Bc_, scalar& hw_, scalarList& p_Xw_)
{
  if (debug_ && faceI==0) {
    Info << "--- compute Surface Mass Balance" << endl;
  }

  double pw = pw_;
  double Tw = Tw_;
  double Bg = Bg_;
  double Bc = 0;
  double hw = 0;
  double * Yke = new double[ne_Bprime];
  double * Ykg = new double[ne_Bprime];
  for(int i = 0; i < ne_Bprime; i++) {
    Yke[i] = Yke_[i];
    Ykg[i] = Ykg_[i];
  }
  double * Xw = new double[ns_Bprime];
  for(int i=0; i<ns_Bprime; i++) {
    Xw[i]=0;
  }

  if (coatingRecession_.boundaryField()[currentPatchID_][faceI]<=coatingHeight_) {
    BprimeTable& BprimeT_ = *coatingBprimeT_ptr;
    if (debug_ && faceI==0) {
      Info << "--- update Bc from " << nameBprimeFile_ << endl;
    }
    Bc = BprimeT_.BprimeC(pw, Bg, Tw);
    if (debug_ && faceI==0) {
      Info << "--- update hw from " << nameBprimeFile_ << endl;
    }
    hw = BprimeT_.hw();
  } else {
    BprimeTable& BprimeT_ = *BprimeT_ptr;
    if (debug_ && faceI==0) {
      Info << "--- update Bc from " << nameBprimeFile_ << endl;
    }
    Bc = BprimeT_.BprimeC(pw, Bg, Tw);
    if (debug_ && faceI==0) {
      Info << "--- update hw from " << nameBprimeFile_ << endl;
    }
    hw = BprimeT_.hw();
  }
  if (debug_ && faceI==0) {
    Info << "--- update X_w" << endl;
  }
  p_Xw_.resize(ns_Bprime);
  for(int i=0; i<ns_Bprime; i++) {
    p_Xw_[i] = Xw[i];
  }
  Bc_=Bc;
  hw_=hw;
}

void Foam::BprimeCoatingBoundaryConditions::write(Ostream& os) const
{
  if (boundaryMapping_ptr) {
    os << "\t// --- start --- Boundary Mapping Inputs" << endl;
    boundaryMapping_ptr->write(os);
    os << "\t// --- end --- Boundary Mapping Inputs" << endl;
  }

  os.writeKeyword("BprimeFile") << nameBprimeFile_ << token::END_STATEMENT << nl;
  os.writeKeyword("coatingBprimeFileName") << coatingBprimeFileName_ << token::END_STATEMENT << nl;
  os.writeKeyword("coatingHeight") << coatingHeight_ << token::END_STATEMENT << nl;
  os.writeKeyword("coatingTableFileName") << coatingTableFileName_ << token::END_STATEMENT << nl;

}

// ************************************************************************* //
