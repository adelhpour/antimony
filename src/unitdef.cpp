#include <cassert>
#include <iostream>
#include <cstdlib>
#include <set>

#include "stringx.h"
#include "sbmlx.h"
#include "unitdef.h"
#include "registry.h"

using namespace std;

UnitDef::UnitDef(string name, string modname)
  : m_components()
  , m_name()
  , m_nameautogenerated(false)
  , m_module(modname)
{
  FixUnitName(name);
  m_name.push_back(name);
  UnitElement ue(name);
  m_components.push_back(ue);
}

UnitDef::UnitDef(vector<string> name, string modname)
  : m_components()
  , m_name(name)
  , m_nameautogenerated(false)
  , m_module(modname)
{
  assert(name.size()>0);
  UnitElement ue(name[name.size()-1]);
  m_components.push_back(ue);
}

void UnitDef::SetName(vector<string> name)
{
  m_name = name;
  m_nameautogenerated = false;
}

void UnitDef::MultiplyUnitDef(UnitDef* unitdef)
{
  string cc = g_registry.GetCC();
  string name = GetNameDelimitedBy(cc) + "_" + unitdef->GetNameDelimitedBy(cc);
  m_name.clear();
  m_name.push_back(name);
  m_nameautogenerated = true;
  for (size_t ue=0; ue<unitdef->m_components.size(); ue++) {
    AddUnitElement(unitdef->m_components[ue]);
  }
}

void UnitDef::DivideUnitDef(UnitDef* unitdef)
{
  string cc = g_registry.GetCC();
  string name = GetNameDelimitedBy(cc) + "_per_" + unitdef->GetNameDelimitedBy(cc);
  m_name.clear();
  m_name.push_back(name);
  m_nameautogenerated = true;
  for (size_t ue=0; ue<unitdef->m_components.size(); ue++) {
    UnitElement unitel = unitdef->m_components[ue];
    unitel.SetExponent(-unitel.GetExponent());
    AddUnitElement(unitel);
  }
}

void UnitDef::RaiseTo(double pow)
{
  string cc = g_registry.GetCC();
  string name = GetNameDelimitedBy(cc) + DoubleToString(pow);
  m_name.clear();
  m_name.push_back(name);
  m_nameautogenerated = true;
  for (size_t ue=0; ue<m_components.size(); ue++) {
    double newexp = pow * m_components[ue].GetExponent();
    m_components[ue].SetExponent(newexp);
  }
}

void UnitDef::MultiplyBy(double mult)
{
  string cc = g_registry.GetCC();
  string name = "_" + DoubleToString(mult) + "_" + GetNameDelimitedBy(cc);
  m_name.clear();
  m_name.push_back(name);
  m_nameautogenerated = true;
  assert(m_components.size() > 0);
  mult = pow(mult, 1.0/m_components[0].GetExponent());
  m_components[0].MultiplyBy(mult);
}

void UnitDef::Reduce()
{
  if (m_components.size() <=2) return;
  for (vector<UnitElement>::iterator first = m_components.begin(); first!= m_components.end();first++)
  {
    vector<UnitElement>::iterator match = first;
    match++;
    while (match != m_components.end()) {
      if (first->GetExponent() == -match->GetExponent() &&
          first->GetKind() == match->GetKind() &&
          first->GetMultiplier() == match->GetMultiplier() &&
          first->GetScale() == match->GetScale()) {
            m_components.erase(match);
            m_components.erase(first);
            match = m_components.end();
            first = m_components.begin();
      }
      else {
        match++;
      }
    }
  }

}

void UnitDef::AddUnitElement(const UnitElement& ue)
{
  m_components.push_back(ue);
}

void UnitDef::ClearComponents()
{
  m_components.clear();
}

bool UnitDef::GetNameAutoGenerated()
{
  return m_nameautogenerated;
}

bool UnitDef::IsEmpty()
{
  return (m_components.empty());
}

bool UnitDef::ClearReferencesTo(Variable* deletedvar)
{
  vector<string> delname = deletedvar->GetName();
  for (size_t el=0; el<m_components.size(); el++) {
    if (m_components[el].GetKind() == delname[delname.size()-1]) {
      ClearComponents();
      return true;
    }
  }
  return false;
}

bool UnitDef::SetFromFormula(Formula* formula)
{
#ifndef NSBML
  ASTNode* astn = parseStringToASTNode(formula->ToSBMLString());
  UnitDef* ud = GetUnitDefFromASTNode(astn);
  if (ud==NULL) {
    g_registry.SetError("Unable to set a unit definition using the formula '" + formula->ToDelimitedStringWithEllipses(".") + "'.  Only multiplication, division, and raising a value to a numerical power are allowed, and no 'bare' dimensions are allowed (use 'dimensionless' explicitly).");
    return true;
  }
  m_components = ud->m_components;
  if (m_name[0]=="") {
    m_name = ud->m_name;
    m_nameautogenerated = true;
  }
  delete ud;
  return false;
#else
  g_registry.SetError("Error:  libsbml required to declare unit definitions in this way.  Use 'unit [unit] = [definition]' instead.");
  return true;
#endif
}

#ifndef NSBML
UnitDef*  UnitDef::GetUnitDefFromASTNode(ASTNode* astn)
{
  if (astn==NULL) return NULL;
  UnitDef* ret = NULL;
  ASTNode* child1 = astn->getChild(0);
  ASTNode* child2 = astn->getChild(1);
  UnitDef* udch1 = GetUnitDefFromASTNode(child1);
  UnitDef* udch2 = GetUnitDefFromASTNode(child2);
  switch(astn->getType()) {
  case AST_TIMES:
    if (astn->getNumChildren()==0) break;
    if (astn->getNumChildren()==1) return udch1;
    if ((child1->isReal() || child1->isInteger()) && (!child2->isReal() && !child2->isInteger())) {
      if (udch2==NULL) break;
      udch2->MultiplyBy(GetValueFrom(child1));
      ret = udch2;
      delete udch1;
    }
    else if ((child2->isReal() || child2->isInteger()) && (!child1->isReal() && !child1->isInteger())) {
      if (udch1==NULL) break;
      udch1->MultiplyBy(1/GetValueFrom(child2));
      ret =  udch1;
      delete udch2;
    }
    else if (udch1==NULL || udch2==NULL) break; //Some sort of error?
    else {
      ret = udch1;
      ret->MultiplyUnitDef(udch2);
      delete udch2;
    }
    //Might be 3+ children of astn, so delete the two we have and call recursively:
    if (astn->getNumChildren() > 2) {
      astn->removeChild(0);
      astn->removeChild(0);
      udch1 = GetUnitDefFromASTNode(astn);
      if (udch1 == NULL) {
        delete ret;
        ret=NULL;
        break;
      }
      ret->MultiplyUnitDef(udch1);
      delete udch1;
    }
    return ret;
    break;
  case AST_DIVIDE:
    if (astn->getNumChildren()!=2) break;
    if (udch1 != NULL) {
      if (udch2 != NULL) {
        udch1->DivideUnitDef(udch2);
        delete udch2;
        return udch1;
      }
      if (child2->isReal() || child2->isInteger()) {
        if (udch1==NULL) break;
        udch1->MultiplyBy(1/GetValueFrom(child2));
        delete udch2;
        return udch1;
      }
      //if udch2 is NULL and not a number, it's an error.
      delete udch1;
      return NULL;
    }
    if (udch2 != NULL) {
      if (child1->isReal() || child1->isInteger()) {
        udch2->Invert();
        udch2->MultiplyBy(GetValueFrom(child1));
        delete udch1;
        return udch2;
      }
      //If udch1 is NULL and not a number, it's an error
      delete udch2;
      return NULL;
    }
    //Both of them are NULL.  It's possible it was a number divided by another number, but that's not legal.
    return NULL;
  case AST_POWER:
  case AST_FUNCTION_POWER:
    if (astn->getNumChildren()!=2) break;
    if (udch1==NULL) break;
    if (child2->isReal() || child2->isInteger()) {
      udch1->RaiseTo(GetValueFrom(child2));
      return udch1;
    }
    break;
  case AST_NAME:
    ret = new UnitDef(astn->getName(), m_module);
    break;
  case AST_REAL:
  case AST_RATIONAL:
  case AST_REAL_E:
  case AST_INTEGER:
    if (astn->isSetUnits()) {
      ret = new UnitDef(astn->getUnits(), m_module);
      ret->MultiplyBy(GetValueFrom(astn));
    }
    break;
  default:
    //Nothing; leave it NULL.
    break;
  }
  delete udch1;
  delete udch2;
  return ret;
}
#endif

bool UnitDef::Matches(UnitDef* unitdef)
{
  if (!m_nameautogenerated && m_name != unitdef->m_name) return false;
  return ComponentsMatch(unitdef);
}

set<UnitElement> GetSetFrom(vector<UnitElement> unitelements) {
  set<UnitElement> ret;
  for (size_t ue=0; ue<unitelements.size(); ue++) {
    ret.insert(unitelements[ue]);
  }
  return ret;
}

bool UnitDef::ComponentsMatch(UnitDef* unitdef)
{
  set<UnitElement> orig = GetSetFrom(m_components);
  set<UnitElement> match = GetSetFrom(unitdef->m_components);
  for (set<UnitElement>::iterator origel = orig.begin(); origel != orig.end();) {
    bool foundmatch = false;
    for (set<UnitElement>::iterator matchel = match.begin(); matchel != match.end();) {
      if (origel->Matches(*matchel)) {
        match.erase(matchel);
        foundmatch = true;
        break;
      }
      matchel++;
    }
    if (foundmatch) {
      set<UnitElement>::iterator remove = origel;
      origel++;
      orig.erase(remove);
    }
    else {
      origel++;
    }
  }
  if (orig.size()==0 && match.size()==0) return true;
  return false;
}

bool UnitDef::IsOnlyCanonicalKind() const
{
  string cc = g_registry.GetCC();
  set<string> usednames;
  const UnitDef* canonical = GetCanonical(usednames);
  if (canonical==NULL) {
    return false;
  }
  if (canonical->m_components.size() != 1) {
    delete canonical;
    return false;
  }
  UnitElement ue = canonical->m_components[0];
  delete canonical;
  if (ue.GetExponent() != 1) return false;
  if (ue.GetMultiplier() != 1) return false;
  if (ue.GetScale() != 0) return false;
  if (ue.GetKind() != GetNameDelimitedBy(cc)) return false;
  return ue.KindIsCanonical();
}


unsigned long UnitDef::GetNumUnitElements() const
{
  return m_components.size();
}

UnitElement* UnitDef::GetUnitElement(unsigned long n)
{
  if (n >= m_components.size()) return NULL;
  return &m_components[n];
}

vector<string> UnitDef::GetName() const
{
  return m_name;
}

string UnitDef::GetNameDelimitedBy(std::string cc) const
{
  return ToStringFromVecDelimitedBy(m_name, cc);
}

string UnitDef::ToStringDelimitedBy(std::string cc) const
{
  stringstream ret;
  vector<UnitElement> tops;
  vector<UnitElement> bottoms;
  for (size_t ue=0; ue<m_components.size(); ue++) {
    if (m_components[ue].GetExponent() > 0) {
      tops.push_back(m_components[ue]);
    }
    else {
      bottoms.push_back(m_components[ue]);
    }
  }
  for (size_t top=0; top<tops.size(); top++) {
    if (top != 0) {
      ret << " * ";
    }
    ret << tops[top].ToString();
  }
  if (tops.size()==0) {
    ret << "1";
  }
  for (size_t bottom=0; bottom<bottoms.size(); bottom++) {
    if (bottom==0) {
      ret << " / ";
      if (bottoms.size()>1) ret << "(";
    }
    else {
      ret << " * ";
    }
    ret << bottoms[bottom].ToInvString();
    if (bottoms.size() > 1 && bottom==bottoms.size()-1) {
      ret << ")";
    }
  }
  return ret.str();
}

void UnitDef::Invert()
{
  string cc = g_registry.GetCC();
  string name = "inv_" + GetNameDelimitedBy(cc);
  m_name.clear();
  m_name.push_back(name);
  for (size_t ue=0; ue<m_components.size(); ue++) {
    m_components[ue].Invert();
  }
}

UnitDef* UnitDef::GetCanonical() const
{
  set<string> usednames;
  return GetCanonical(usednames);
}

UnitDef* UnitDef::GetCanonical(set<string> usednames) const
{
  bool sbmlkindsonly = true;
  for (size_t ue=0; ue<m_components.size(); ue++) {
    if (!m_components[ue].KindIsCanonical()) {
      sbmlkindsonly = false;
      break;
    }
  }
  if (sbmlkindsonly) return new UnitDef(*this);
  UnitDef* ret = new UnitDef(m_name, m_module);
  ret->m_components.clear();
  for (size_t ue=0; ue<m_components.size(); ue++) {
    if (m_components[ue].KindIsCanonical()) {
      ret->AddUnitElement(m_components[ue]);
    }
    else {
      UnitElement unitelement = m_components[ue];
      string subname = unitelement.GetKind();
      if (usednames.find(subname) != usednames.end()) {
        g_registry.SetError("Loop detected in unit definitions with '" + subname + "'.");
        delete ret;
        return NULL;
      }
      usednames.insert(subname);
      vector<string> unitname;
      unitname.push_back(subname);
      Variable* var = g_registry.GetModule(m_module)->GetVariable(unitname);
      if (var==NULL) {
        g_registry.SetError("Undefined unit definition '" + subname + "'.");
        delete ret;
        return NULL;
      }
      UnitDef* ud = var->GetUnitDef()->GetCanonical(usednames);
      if (ud==NULL) {
        //Should already have set the error;
        delete ret;
        return NULL;
      }
      ud->MultiplyBy(unitelement.GetMultiplier());
      double ten=10;
      ud->MultiplyBy(pow(ten, unitelement.GetScale()));
      ud->RaiseTo(unitelement.GetExponent());
      ret->MultiplyUnitDef(ud);
      delete ud;
    }
  }
  return ret;
}

#ifndef NSBML
UnitDefinition* UnitDef::AddToSBML(Model* sbmlmod, string id, string name)
{
  UnitDef* canonical = GetCanonical();
  if (canonical!=NULL && canonical->IsOnlyCanonicalKind()) {
    delete canonical;
    return NULL;
  }
  UnitDefinition* ud = sbmlmod->createUnitDefinition();
  if (id=="time_unit") id = "time";
  ud->setId(id); //Don't use any auto-generated names.
  ud->setName(name);
  if (canonical==NULL) return NULL;
  for (size_t ue=0; ue<canonical->m_components.size(); ue++) {
    UnitElement unitel = canonical->m_components[ue];
    Unit* unit = ud->createUnit();
    UnitKind_t kind = UnitKind_forName(unitel.GetKind().c_str());
    if (kind==UNIT_KIND_METER) kind = UNIT_KIND_METRE;
    unit->setKind(kind);
    unit->setExponent(unitel.GetExponent());
    unit->setMultiplier(unitel.GetMultiplier());
    unit->setScale(unitel.GetScale());
  }
  delete canonical;
  return ud;
}
#endif
