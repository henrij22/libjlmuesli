
#include "finitestrain.hh"
#include "materialproperties.hh"
#include "materialstate.hh"
#include "smallstrain.hh"

#include <muesli/Finitestrain/finitestrain.h>
#include <muesli/Finitestrain/neohook.h>
#include <muesli/Math/mtensor.h>
#include <muesli/muesli.h>
#include <muesli/Smallstrain/elastic.h>
#include <muesli/Smallstrain/smallstrain.h>
#include <muesli/Utils/utils.h>

#include <jlcxx/jlcxx.hpp>
#include <jlcxx/type_conversion.hpp>

JLCXX_MODULE define_julia_module(jlcxx::Module& mod) {
  using jlcxx::arg;
  using jlcxx::julia_base_type;

  // Register utils
  registerMaterialProperties(mod);
  registerMaterialState(mod);

  // Register base classes (important?)
  mod.add_type<muesli::material>("Material");
  mod.add_type<muesli::materialPoint>("MaterialPoint");

  // Linear elasticity
  {
    using Material      = muesli::elasticIsotropicMaterial;
    using MaterialPoint = muesli::elasticIsotropicMP;

    auto mat = registerSmallStrainMaterial<Material, MaterialPoint>(mod, "ElasticIsotropic");

    mat.constructor(
        [](double Emod, double nu, double rho = 1.0) { return new Material{"ElasticIsotropic", Emod, nu, rho}; },
        arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }
  {
    mod.add_type<muesli::finiteStrainMaterial>("FiniteStrainMaterial", julia_base_type<muesli::material>());
    mod.add_type<muesli::f_invariants>("F_invariants", julia_base_type<muesli::finiteStrainMaterial>());

    mod.add_type<muesli::finiteStrainMP>("FiniteStrainMP", julia_base_type<muesli::materialPoint>());
    mod.add_type<muesli::fisotropicMP>("FisotropicMP", julia_base_type<muesli::finiteStrainMP>());

    using Material      = muesli::neohookeanMaterial;
    using MaterialPoint = muesli::neohookeanMP;
    auto mat = registerFiniteStrainMaterial<Material, MaterialPoint, muesli::f_invariants, muesli::fisotropicMP>(
        mod, "NeoHooke");
    mat.constructor(
        [](double Emod, double nu, double rho = 1.0) { return new Material{"ElasticIsotropic", Emod, nu, rho}; },
        arg("Emod"), arg("nu"), arg("rho") = 1.0);
  }
}

// JLCXX_MODULE define_types_module(jlcxx::Module& types)
// {
//   types.add_bits<muesli::propertyName>("PropertyName",
//   jlcxx::julia_type("CppEnum")); types.set_const("PR_ALPHA",
//   muesli::propertyName::PR_ALPHA);
// }