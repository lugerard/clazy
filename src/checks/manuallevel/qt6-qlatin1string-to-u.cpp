/*
    This file is part of the clazy static checker.

    Copyright (C) 2020 The Qt Company Ltd.
    Copyright (C) 2020 Lucie Gerard <lucie.gerard@qt.io>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "qt6-qlatin1string-to-u.h"
#include "ClazyContext.h"
#include "Utils.h"
#include "StringUtils.h"
#include "FixItUtils.h"
#include "HierarchyUtils.h"
#include "SourceCompatibilityHelpers.h"
#include "clazy_stl.h"

#include <clang/Lex/Lexer.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Casting.h>

using namespace clang;
using namespace std;

Qt6QLatin1StringToU::Qt6QLatin1StringToU(const std::string &name, ClazyContext *context)
    : CheckBase(name, context, Option_CanIgnoreIncludes)
{
}

static bool isInterestingParam(ParmVarDecl *param, bool &is_char_array)
{
    is_char_array = false;
    const string typeStr = param->getType().getAsString();
    if (typeStr == "const char *") {// We only want const char*
        is_char_array = true;
    }

    return is_char_array;
}

static bool isInterestingCtorCall(CXXConstructorDecl *ctor, bool &is_char_array)
{
    is_char_array = false;
    if (!ctor || !clazy::isOfClass(ctor, "QLatin1String"))
        return false;

    for (auto param : Utils::functionParameters(ctor)) {
        if (isInterestingParam(param, is_char_array))
            break;
        return false;
    }

    return is_char_array;
}

void Qt6QLatin1StringToU::VisitStmt(clang::Stmt *stm)
{
    CXXConstructExpr *ctorExpr = dyn_cast<CXXConstructExpr>(stm);
    if (!ctorExpr)
        return;

    vector<FixItHint> fixits;
    bool is_char_array = false;
    string methodName;
    string message;

    if (ctorExpr) {
        CXXConstructorDecl *ctorDecl = ctorExpr->getConstructor();
        if (!isInterestingCtorCall(ctorDecl, is_char_array))
            return;
        if (is_char_array) {
            message = "QLatin1String(const char *) ctor being called";
        }
        fixits = fixitReplace(ctorExpr);
    } else {
        return;
    }

    emitWarning(clazy::getLocStart(stm), message, fixits);
}

std::vector<FixItHint> Qt6QLatin1StringToU::fixitReplace(CXXConstructExpr *ctorExpr)
{
    const string replacement = "u";
    const string replacee = "QLatin1String";
    vector<FixItHint> fixits;

    SourceLocation rangeStart = clazy::getLocStart(ctorExpr);
    SourceLocation rangeEnd = Lexer::getLocForEndOfToken(rangeStart, -1, sm(), lo());

    if (rangeEnd.isInvalid()) {
        // Fallback. Have seen a case in the wild where the above would fail, it's very rare
        rangeEnd = rangeStart.getLocWithOffset(replacee.size() - 2);
        if (rangeEnd.isInvalid()) {
            clazy::printLocation(sm(), rangeStart);
            clazy::printLocation(sm(), rangeEnd);
            clazy::printLocation(sm(), Lexer::getLocForEndOfToken(rangeStart, 0, sm(), lo()));
            queueManualFixitWarning(clazy::getLocStart(ctorExpr));
            return {};
        }
    }

    fixits.push_back(FixItHint::CreateReplacement(SourceRange(rangeStart, rangeEnd), replacement));
    return fixits;
}
