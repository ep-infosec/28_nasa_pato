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
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "psiThermoFrozen.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(psiThermoFrozen, 0);
    defineRunTimeSelectionTable(psiThermoFrozen, fvMesh);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::psiThermoFrozen::psiThermoFrozen(const fvMesh& mesh, const word& phaseName)
:
    fluidThermo(mesh, phaseName),

    psi_
    (
        IOobject
        (
            phasePropertyName("thermo:psi"),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionSet(0, -2, 2, 0, 0)
    ),

    mu_
    (
        IOobject
        (
            phasePropertyName("thermo:mu"),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionSet(1, -1, -1, 0, 0)
    )
{}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::psiThermoFrozen> Foam::psiThermoFrozen::New
(
    const fvMesh& mesh,
    const word& phaseName
)
{
    return basicThermo::New<psiThermoFrozen>(mesh, phaseName);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::psiThermoFrozen::~psiThermoFrozen()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::volScalarField> Foam::psiThermoFrozen::rho() const
{
    return p_*psi_;
}


Foam::tmp<Foam::scalarField> Foam::psiThermoFrozen::rho(const label patchi) const
{
    return p_.boundaryField()[patchi]*psi_.boundaryField()[patchi];
}


const Foam::volScalarField& Foam::psiThermoFrozen::psi() const
{
    return psi_;
}


Foam::tmp<Foam::volScalarField> Foam::psiThermoFrozen::mu() const
{
    return mu_;
}


Foam::tmp<Foam::scalarField> Foam::psiThermoFrozen::mu(const label patchi) const
{
    return mu_.boundaryField()[patchi];
}


// ************************************************************************* //
