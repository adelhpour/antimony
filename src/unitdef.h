#ifndef UNITDEF_H
#define UNITDEF_H

#include <string>
#include <utility>
#include <vector>
#include "unitelement.h"

class ReactantList;
class Variable;
#ifndef NSBML
class Model;
#endif

class UnitDef
{
private:
  //UnitDef(const UnitDef& src); //Undefined
  //UnitDef& operator=(const UnitDef& src); //Undefined

  std::vector<UnitElement> m_components;
  std::string m_module; //The name of the high-level module we're in.
  std::vector<std::string> m_name;
  bool m_nameautogenerated;

public:
  UnitDef(std::string name, std::string modname);
  UnitDef(std::vector<std::string> name, std::string modname);
  ~UnitDef() {};

  void SetName(std::vector<std::string> name);
  void MultiplyUnitDef(UnitDef* unitdef);
  void DivideUnitDef(UnitDef* unitdef);
  void RaiseTo(double pow);
  void MultiplyBy(double mult);
  void Reduce();
  void AddUnitElement(const UnitElement& ue);
  void ClearComponents();
  bool GetNameAutoGenerated();
  bool IsEmpty();
  bool ClearReferencesTo(Variable* deletedvar);


  bool SetFromFormula(Formula* formula);
  bool Matches(UnitDef* unitdef);
  bool ComponentsMatch(UnitDef* unitdef);
  bool IsOnlyCanonicalKind() const;

  unsigned long GetNumUnitElements() const;
  UnitElement* GetUnitElement(unsigned long n);

  std::vector<std::string> GetName() const;
  std::string GetNameDelimitedBy(std::string cc) const;
  std::string ToStringDelimitedBy(std::string cc) const;
  void Invert();
  UnitDef* GetCanonical() const;
  UnitDef* GetCanonical(std::set<std::string> usednames) const;
#ifndef NSBML
  UnitDef* GetUnitDefFromASTNode(ASTNode* astn);
  UnitDefinition* AddToSBML(Model* sbmlmod, std::string id, std::string name);
#endif

private:

};


#endif //UNITDEF_H
