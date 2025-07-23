//
// Created by Declan Walsh on 9/05/2024.
//

#include "Variable.h"

bool VariableInfo::IsHeadMemberOf(const std::string &instance) const
{
    return !ParentInstance.empty() && ParentInstance == instance && MemberIndex == 0;
}

bool VariableInfo::IsClassHead() const
{
    return !ParentClass.empty() && MemberIndex == 0;
}

bool VariableInfo::Match(const std::string &name, const std::string &parent)
{
    if (ParentInstance.empty() && parent.empty() && Name == name) {
        return true;
    } else if (!ParentInstance.empty() && ParentInstance == parent && Name == name) {
        return true;
    }
    return false;
}
