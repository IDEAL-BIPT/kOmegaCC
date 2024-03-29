/*---------------------------------------------------------------------------*\
  =========                 |
   \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2010 OpenCFD Ltd.
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

#include "kOmegaCC.H"
#include "addToRunTimeSelectionTable.H"

#include "backwardsCompatibilityWallFunctions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace compressible
{
namespace RASModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(kOmegaCC, 0);
addToRunTimeSelectionTable(RASModel, kOmegaCC, dictionary);

// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

tmp<volScalarField> kOmegaCC::F1(const volScalarField& CDkOmega) const
{
    volScalarField CDkOmegaPlus = max
    (
        CDkOmega,
        dimensionedScalar("1.0e-10", dimless/sqr(dimTime), 1.0e-10)
    );

    volScalarField arg1 = min
    (
        min
        (
            max
            (
                (scalar(1)/betaStar_)*sqrt(k_)/(omega_*y_),
                scalar(500)*(mu()/rho_)/(sqr(y_)*omega_)
            ),
            (4*alphaOmega2_)*k_/(CDkOmegaPlus*sqr(y_))
        ),
        scalar(10)
    );

    return tanh(pow4(arg1));
}

tmp<volScalarField> kOmegaCC::F2() const
{
    volScalarField arg2 = min
    (
        max
        (
            (scalar(2)/betaStar_)*sqrt(k_)/(omega_*y_),
            scalar(500)*(mu()/rho_)/(sqr(y_)*omega_)
        ),
        scalar(100)
    );

    return tanh(sqr(arg2));
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

kOmegaCC::kOmegaCC
(
    const volScalarField& rho,
    const volVectorField& U,
    const surfaceScalarField& phi,
    const basicThermo& thermophysicalModel
)
:
    RASModel(typeName, rho, U, phi, thermophysicalModel),

    alphaK1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaK1",
            coeffDict_,
            0.85034
        )
    ),
    alphaK2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaK2",
            coeffDict_,
            1.0
        )
    ),
    alphaOmega1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaOmega1",
            coeffDict_,
            0.5
        )
    ),
    alphaOmega2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaOmega2",
            coeffDict_,
            0.85616
        )
    ),
    Prt_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "Prt",
            coeffDict_,
            1.0
        )
    ),
    gamma1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma1",
            coeffDict_,
            0.5532
        )
    ),
    gamma2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma2",
            coeffDict_,
            0.4403
        )
    ),
    beta1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta1",
            coeffDict_,
            0.075
        )
    ),
    beta2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta2",
            coeffDict_,
            0.0828
        )
    ),
    betaStar_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "betaStar",
            coeffDict_,
            0.09
        )
    ),
    a1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "a1",
            coeffDict_,
            0.31
        )
    ),
    cr1_ // this class and next two was added by me (model coefficients)
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr1",
            coeffDict_,
            1.0
        )
    ),
    cr2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr2",
            coeffDict_,
            2.0
        )
    ),
    cr3_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr3",
            coeffDict_,
            1.0
        )
    ),
    c1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "c1",
            coeffDict_,
            10.0
        )
    ),

    y_(mesh_),

    k_
    (
        IOobject
        (
            "k",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateK("k", mesh_)
    ),
    omega_
    (
        IOobject
        (
            "omega",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateOmega("omega", mesh_)
    ),
    mut_
    (
        IOobject
        (
            "mut",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateMut("mut", mesh_)
    ),
    alphat_
    (
        IOobject
        (
            "alphat",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateAlphat("alphat", mesh_)
    )
{
    bound(omega_, omega0_);

    mut_ = a1_*rho_*k_/max(a1_*omega_, F2()*sqrt(magSqr(symm(fvc::grad(U_)))));
    mut_.correctBoundaryConditions();

    alphat_ = mut_/Prt_;
    alphat_.correctBoundaryConditions();

    printCoeffs();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //
tmp<volSymmTensorField> kOmegaCC::R() const
{
    return tmp<volSymmTensorField>
    (
        new volSymmTensorField
        (
            IOobject
            (
                "R",
                runTime_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            ((2.0/3.0)*I)*k_ - (mut_/rho_)*dev(twoSymm(fvc::grad(U_))),
            k_.boundaryField().types()
        )
    );
}


tmp<volSymmTensorField> kOmegaCC::devRhoReff() const
{
    return tmp<volSymmTensorField>
    (
        new volSymmTensorField
        (
            IOobject
            (
                "devRhoReff",
                runTime_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            -muEff()*dev(twoSymm(fvc::grad(U_)))
        )
    );
}


tmp<fvVectorMatrix> kOmegaCC::divDevRhoReff(volVectorField& U) const
{
    return
    (
      - fvm::laplacian(muEff(), U) - fvc::div(muEff()*dev2(fvc::grad(U)().T()))
    );
}


bool kOmegaCC::read()
{
    if (RASModel::read())
    {
        alphaK1_.readIfPresent(coeffDict());
        alphaK2_.readIfPresent(coeffDict());
        alphaOmega1_.readIfPresent(coeffDict());
        alphaOmega2_.readIfPresent(coeffDict());
        Prt_.readIfPresent(coeffDict());
        gamma1_.readIfPresent(coeffDict());
        gamma2_.readIfPresent(coeffDict());
        beta1_.readIfPresent(coeffDict());
        beta2_.readIfPresent(coeffDict());
        betaStar_.readIfPresent(coeffDict());
        a1_.readIfPresent(coeffDict());
        c1_.readIfPresent(coeffDict());

        return true;
    }
    else
    {
        return false;
    }
}


void kOmegaCC::correct()
{
    if (!turbulence_)
    {
        // Re-calculate viscosity
        mut_ =
            a1_*rho_*k_
           /max(a1_*omega_, F2()*sqrt(magSqr(symm(fvc::grad(U_)))));
        mut_.correctBoundaryConditions();

        // Re-calculate thermal diffusivity
        alphat_ = mut_/Prt_;
        alphat_.correctBoundaryConditions();

        return;
    }

    RASModel::correct();

    volScalarField divU = fvc::div(phi_/fvc::interpolate(rho_));

    if (mesh_.changing())
    {
        y_.correct();
    }

    if (mesh_.moving())
    {
        divU += fvc::div(mesh_.phi());
    }

    tmp<volTensorField> tgradU = fvc::grad(U_);
    tmp<volTensorField> tSkew = skew(tgradU()); //Was added by me (antisymmetric part of the StressTensor)
    tmp<volSymmTensorField> tSymm = symm(tgradU()); //Was added by me (symmetric part of the StressTensor)
    volScalarField symInnerProduct = 2. * tSymm() && tSymm();
    volScalarField asymInnerProduct = max(2. * tSkew() && tSkew(), dimensionedScalar("1e-16", dimensionSet(0, 0, -2, 0, 0), 1e-10) );
    volScalarField S2 = magSqr(symm(tgradU()));
    volScalarField GbyMu = (tgradU() && dev(twoSymm(tgradU())));
    volScalarField rStar = sqrt(symInnerProduct/asymInnerProduct);
    volScalarField G("RASModel::G", mut_*GbyMu);
        tgradU.clear();
    // Update omega and G at the wall
    omega_.boundaryField().updateCoeffs();
    volScalarField D = sqrt(max(asymInnerProduct, 0.09*omega_*omega_)); //Possibly wrong. Don't know what Omega is used in equation
    tmp<volSymmTensorField> divS = fvc::ddt(tSymm()) + fvc::div(surfaceScalarField("phiU",phi_/fvc::interpolate(rho_)), tSymm()); //Was added by me (Substantional Derivative of the StressTensor symmetric part)
    volScalarField rT = tSkew().component(0)*tSymm().component(0)*divS().component(0) +
                            tSkew().component(0)*tSymm().component(1)*divS().component(1) +
                            tSkew().component(0)*tSymm().component(2)*divS().component(2) +
                            tSkew().component(3)*tSymm().component(0)*divS().component(3) +
                            tSkew().component(3)*tSymm().component(1)*divS().component(4) +
                            tSkew().component(3)*tSymm().component(2)*divS().component(5) +
                            tSkew().component(6)*tSymm().component(0)*divS().component(6) +
                            tSkew().component(6)*tSymm().component(1)*divS().component(7) +
                            tSkew().component(6)*tSymm().component(2)*divS().component(8) +
                            tSkew().component(1)*tSymm().component(3)*divS().component(0) +
                            tSkew().component(1)*tSymm().component(4)*divS().component(1) +
                            tSkew().component(1)*tSymm().component(5)*divS().component(2) +
                            tSkew().component(4)*tSymm().component(3)*divS().component(3) +
                            tSkew().component(4)*tSymm().component(4)*divS().component(4) +
                            tSkew().component(4)*tSymm().component(5)*divS().component(5) +
                            tSkew().component(7)*tSymm().component(3)*divS().component(6) +
                            tSkew().component(7)*tSymm().component(4)*divS().component(7) +
                            tSkew().component(7)*tSymm().component(5)*divS().component(8) +
                            tSkew().component(2)*tSymm().component(6)*divS().component(0) +
                            tSkew().component(2)*tSymm().component(7)*divS().component(1) +
                            tSkew().component(2)*tSymm().component(8)*divS().component(2) +
                            tSkew().component(7)*tSymm().component(6)*divS().component(3) +
                            tSkew().component(7)*tSymm().component(7)*divS().component(4) +
                            tSkew().component(7)*tSymm().component(8)*divS().component(5) +
                            tSkew().component(8)*tSymm().component(6)*divS().component(6) +
                            tSkew().component(8)*tSymm().component(7)*divS().component(7) +
                            tSkew().component(8)*tSymm().component(8)*divS().component(8);
    divS.clear();
    tSkew.clear();
    tSymm.clear();
    volScalarField rTilda = 2. * rT/sqrt(asymInnerProduct)/D/D/D;
    volScalarField Frot(
        IOobject(
            "Frot",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        max(min((scalar(1) + cr1_)*2*rStar/(scalar(1)+rStar)*(scalar(1)-cr3_*atan(cr2_*rTilda))-cr1_, scalar(1.25)), scalar(0))
    );
    if(runTime_.outputTime())
    {
        Frot.write();
    }
    rStar.clear();
    rTilda.clear();
    rT.clear();
    D.clear();
    volScalarField CDkOmega =
        (2*alphaOmega2_)*(fvc::grad(k_) & fvc::grad(omega_))/omega_;

    volScalarField F1 = this->F1(CDkOmega);
    volScalarField rhoGammaF1 = rho_*gamma(F1);

    // Turbulent frequency equation
    tmp<fvScalarMatrix> omegaEqn
    (
        fvm::ddt(rho_, omega_)
      + fvm::div(phi_, omega_)
      
      - fvm::laplacian(DomegaEff(F1), omega_)
     ==
        Frot*rhoGammaF1*GbyMu
      - fvm::SuSp((2.0/3.0)*rhoGammaF1*divU, omega_)
      - fvm::Sp(rho_*beta(F1)*omega_, omega_)
      - fvm::SuSp
        (
            rho_*(F1 - scalar(1))*CDkOmega/omega_,
            omega_
        )
    );

    omegaEqn().relax();

    omegaEqn().boundaryManipulate(omega_.boundaryField());

    solve(omegaEqn);
    bound(omega_, omega0_);

    // Turbulent kinetic energy equation
    tmp<fvScalarMatrix> kEqn
    (
        fvm::ddt(rho_, k_)
      + fvm::div(phi_, k_)
      - fvm::laplacian(DkEff(F1), k_)
     ==
        Frot*min(G, (c1_*betaStar_)*rho_*k_*omega_)
      - fvm::SuSp(2.0/3.0*rho_*divU, k_)
      - fvm::Sp(rho_*betaStar_*omega_, k_)
    );

    kEqn().relax();
    solve(kEqn);
    bound(k_, k0_);


    // Re-calculate viscosity
    mut_ = a1_*rho_*k_/max(a1_*omega_, F2()*sqrt(S2));
    mut_.correctBoundaryConditions();

    // Re-calculate thermal diffusivity
    alphat_ = mut_/Prt_;
    alphat_.correctBoundaryConditions();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace compressible
} // End namespace Foam

// ************************************************************************* //
