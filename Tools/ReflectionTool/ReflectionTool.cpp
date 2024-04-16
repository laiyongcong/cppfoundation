#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/Comment.h"
#include "clang/AST/Type.h"
#include "clang/Parse/ParseAST.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Chrono.h"
#include "clang/Lex/Preprocessor.h"
#include <sstream>
#include <set>
#include <vector>
#include <map>
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/ADT/StringExtras.h"

using namespace clang;
using namespace llvm;

std::string OutPutString;
std::string DBFileName = (".cache");
bool bExportEnum = false;
bool bSplitFile = false;
std::string ReflectionString;

StringRef getFileName(SourceLocation src_loc, const SourceManager& SM)
{
	if (!src_loc.isValid())
	{
		return "";
	}
	PresumedLoc PLoc = SM.getPresumedLoc(src_loc);
	if (PLoc.isInvalid())
	{
		return "";
	}

	return PLoc.getFilename();
}

//-------------------------------------------------------------------------------------------------------
struct FVarDecl
{
    std::string mName;
    std::string mType;
    bool mReference;
    bool mConst;
    bool mStatic;

    FVarDecl(std::string& InName, std::string& InType, bool bReferenced, bool bConst, bool bStatic) :
        mName(InName), mType(InType), mReference(bReferenced), mConst(bConst), mStatic(bStatic) {}
};


struct  FFuncMember
{
    bool mVirtual;
    bool mStatic;
    std::string mFuncName;
    std::vector<FVarDecl> mParams;
    bool mIsConst;
    FFuncMember()
    {
        mVirtual = false;
        mStatic = false;
        mFuncName = "";
        mIsConst = false;
    }
};

struct FClassInfo
{
    std::string mName;
    FClassInfo* mParent;
    std::map<std::string, FClassInfo*> mChildren;
    std::map<std::string, FFuncMember> mFuncs;
    std::vector<FVarDecl> mMembers;
    bool mWithConstructor;
    FClassInfo()
    {
        mName = "";
        mParent = NULL;
        mChildren.clear();
        mFuncs.clear();
        mMembers.clear();
        mWithConstructor = false;
    }
};

std::map<std::string, FClassInfo> ClassesInfo;
std::vector<FClassInfo*> ClassesVec;

std::set<std::string> Reflection_HeaderKey;
std::vector<std::string> Reflection_Header;

std::string Reflection_Def;


typedef std::tuple<std::string, llvm::APSInt> name_val_line;
typedef std::pair<std::string, std::vector<name_val_line>> enum_vals_pair;

int EnumCompare(const enum_vals_pair& LHS, const enum_vals_pair& RHS) {
    return LHS.first.compare(RHS.first) < 0;
}
int EnumValCompare(const name_val_line& LHS, const name_val_line& RHS) {
    return std::get<1>(LHS) < std::get<1>(RHS);
}

int ExportClass2Lua(const FClassInfo& info, std::string& strContent)
{
    int ncount = 0;
    char szBuff[512] = {0};
    const std::string& strClass = info.mName;
    if(info.mParent != NULL) 
    {
        snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_add<%s>(pStat, \"Class%s\");\r\n\tlua_tinker::class_inh<%s,%s>(pStat);\r\n",
            strClass.c_str(), strClass.c_str(), strClass.c_str(), info.mParent->mName.c_str());
    } 
    else
    {
        snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_add<%s>(pStat, \"Class%s\");\r\n",strClass.c_str(), strClass.c_str());
    }
    strContent.append(szBuff);
    ncount++;

    if(info.mWithConstructor) 
    {
        snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_con<%s>(pStat, &lua_tinker::constructor<%s>);\r\n",strClass.c_str(), strClass.c_str());
        strContent.append(szBuff);
        ncount++;
    }
    for(auto itf = info.mFuncs.begin(); itf != info.mFuncs.end(); itf++) 
    {
        const FFuncMember& func = itf->second;
        if(func.mStatic) 
        {
            snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_static_def<%s>(pStat, \"%s\", &%s::%s);\r\n",strClass.c_str(), func.mFuncName.c_str(), strClass.c_str(), func.mFuncName.c_str());
        } 
        else 
        {
            snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_def<%s>(pStat, \"%s\", &%s::%s);\r\n",strClass.c_str(), func.mFuncName.c_str(), strClass.c_str(), func.mFuncName.c_str());
        }
        strContent.append(szBuff);
        ncount++;
    }

    for(auto itm = info.mMembers.begin(); itm != info.mMembers.end(); itm++) 
    {
        const std::string& strMember = (*itm).mName;
        snprintf(szBuff, sizeof(szBuff), "\tlua_tinker::class_mem<%s>(pStat, \"%s\", &%s::%s);\r\n",strClass.c_str(), strMember.c_str(), strClass.c_str(), strMember.c_str());
        strContent.append(szBuff);
        ncount++;
    }
    return ncount;
}

int ExportClass2Ref(const FClassInfo& info, std::string& strContent)
{
    int ncount = 0;
    char szBuff[512] = { 0 };
    std::string strClass = info.mName;
    if (info.mParent != NULL)
    {
        snprintf(szBuff, sizeof(szBuff), "static const ReflectiveClass<%s> g_ref%s = ReflectiveClass<%s>(\"%s\", (%s *)0)",
            strClass.c_str(), strClass.c_str(), strClass.c_str(), strClass.c_str(), info.mParent->mName.c_str());
    }
    else
    {
      snprintf(szBuff, sizeof(szBuff), "static const ReflectiveClass<%s> g_ref%s = ReflectiveClass<%s>(\"%s\")",
               strClass.c_str(), strClass.c_str(), strClass.c_str(),strClass.c_str());
    }
    strContent.append(szBuff);
    ncount++;

    for (auto itf = info.mFuncs.begin(); itf != info.mFuncs.end(); itf++)
    {
        const FFuncMember& func = itf->second;
        if (func.mStatic)
        {
            snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefStaticMethod(&%s::%s,\"%s\")",
                strClass.c_str(), func.mFuncName.c_str(), func.mFuncName.c_str());
        }
        else
        {
            if (func.mFuncName == strClass)
            {
                if (func.mParams.empty()) 
                {
                    snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefConstructor()");
                }
                else 
                {
                    std::string strParamTypes = "";
                    for (size_t i = 0, isize = func.mParams.size(); i < isize; i++)
                    {
                        strParamTypes.append(func.mParams[i].mType);
                        if (i != isize - 1)
                        {
                            strParamTypes.append(", ");
                        }
                    }
                    snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefConstructor<%s>()", strParamTypes.c_str());
                }
            }
            else 
            {
                if (func.mIsConst)
                {
                    if (func.mVirtual)
                        snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefConstMethod(&%s::%s,\"%s\", true)", strClass.c_str(), func.mFuncName.c_str(), func.mFuncName.c_str());
                    else
                        snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefConstMethod(&%s::%s,\"%s\")", strClass.c_str(), func.mFuncName.c_str(), func.mFuncName.c_str());
                } 
                else 
                {
                    if (func.mVirtual)
                        snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefMethod(&%s::%s,\"%s\", true)", strClass.c_str(), func.mFuncName.c_str(), func.mFuncName.c_str());
                    else
                      snprintf(szBuff, sizeof(szBuff), "\r\n\t.RefMethod(&%s::%s,\"%s\")", strClass.c_str(), func.mFuncName.c_str(), func.mFuncName.c_str());
                }
            }
            
        }
        strContent.append(szBuff);
        ncount++;
    }
    
    for (auto itm = info.mMembers.begin(); itm != info.mMembers.end(); itm++)
    {
        std::string strMember = (*itm).mName;
        if (itm->mStatic) {
            snprintf(szBuff, sizeof(szBuff),
                     "\r\n\t.RefStaticField(&%s::%s,\"%s\")", strClass.c_str(),
                     strMember.c_str(), strMember.c_str());
        } else {
            snprintf(szBuff, sizeof(szBuff),
                     "\r\n\t.RefField(&%s::%s,\"%s\")", strClass.c_str(),
                     strMember.c_str(), strMember.c_str());
        }
        strContent.append(szBuff);
        ncount++;
    }

    strContent.append(";\r\n");
    ncount++;
    return ncount;
}


void BuildClassInh(std::string strClassName, std::string strParent)
{
    size_t pos = strParent.rfind(":");
    if (pos != std::string::npos)
    {
        strParent = strParent.substr(pos + 1);
    }
    auto it = ClassesInfo.find(strClassName);
    auto itp = ClassesInfo.find(strParent);
    if (it == ClassesInfo.end() || itp == ClassesInfo.end())
    {
        return;
    }
    it->second.mParent = &(itp->second);
    it->second.mParent->mChildren[strClassName] = &(it->second);
}

void AddRefClass(std::string strClassName, CXXRecordDecl *Declaration, bool withLuaConstructor )
{
    if (strClassName[0] == '_')
    {
        return;
    }
    auto it = ClassesInfo.find(strClassName);
    if (it != ClassesInfo.end())
    {
        if(withLuaConstructor)
        {
          it->second.mWithConstructor = true;  
        }
        return;
    }
    FClassInfo info;
    info.mName = strClassName;
    ClassesInfo[info.mName] = info;
    if (Declaration->getNumBases() > 0)
    {
        std::string strParent = Declaration->bases_begin()->getType().getAsString();
        BuildClassInh(strClassName, strParent);
    }
    it = ClassesInfo.find(strClassName);
    ClassesVec.push_back(&(it->second));
}

void AddClassFunc(std::string strClassName, CXXRecordDecl *Declaration, std::string strFunc, bool bStatic, bool bConst, FunctionDecl* func_decl)
{
    auto it = ClassesInfo.find(strClassName);
    if (it == ClassesInfo.end())
    {
        AddRefClass(strClassName, Declaration, false);
    }
    it = ClassesInfo.find(strClassName);
    if (it == ClassesInfo.end())
    {
        return;
    }
    FClassInfo& info = it->second;
    if (info.mFuncs.find(strFunc) != info.mFuncs.end())
    {
        return;
    }

    bool bFound = false;
    FClassInfo* pParent = info.mParent;
    while (pParent)
    {
        auto itfunc = pParent->mFuncs.find(strFunc);
        if (itfunc != pParent->mFuncs.end())
        {
            if (itfunc->second.mVirtual)
            {
                bFound = true;
                break;
            }
        }
        pParent = pParent->mParent;
    }
    if (bFound)
    {
        return;
    }

    FFuncMember funcMem;
    funcMem.mFuncName = strFunc;
    funcMem.mStatic = bStatic;
    funcMem.mIsConst = bConst;
    funcMem.mVirtual = func_decl->isVirtualAsWritten();
   

    for (unsigned i = 0; i < func_decl->getNumParams(); ++i)
    {
        auto param = func_decl->getParamDecl(i);
        std::string name = param->getName().str();
        std::string type = param->getType().getAsString();
        bool bReference = false;
        bool bConst = false;
        if (type.find_first_of("const") == 0)
        {
            bConst = true;
        }
        if (type.find_last_of("&") == type.length() - 1)
        {
            bReference = true;
        }

        funcMem.mParams.push_back(FVarDecl(name, type, bReference, bConst, false));
    }

    info.mFuncs[strFunc] = funcMem;
}

void AddClassMember(std::string strClassName, CXXRecordDecl *Declaration, std::string strMember, std::string strType, bool bStatic)
{
    auto it = ClassesInfo.find(strClassName);
    if (it == ClassesInfo.end())
    {
        AddRefClass(strClassName, Declaration, false);
    }
    it = ClassesInfo.find(strClassName);
    if (it == ClassesInfo.end())
    {
        return;
    }
    FClassInfo& info = it->second;
    info.mMembers.push_back(FVarDecl(strMember, strType, false, false, bStatic));
}

//std::vector<enum_vals_pair> EnumDefines;

class ReflectionStructVisitor
    : public RecursiveASTVisitor<ReflectionStructVisitor> {
public:
    explicit ReflectionStructVisitor(ASTContext *Context)
        : Context(Context) {}

    bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
        
        SmallString<256> file_name = getFileName(Declaration->getLocation(), Context->getSourceManager());
        sys::path::native(file_name);


        static bool bRefExportFlag = false;
        bool bExport = false;
        bool bOldExport = false;
        StringRef class_name = Declaration->getName();
        for (auto decl_iter = Declaration->decls_begin(); decl_iter != Declaration->decls_end(); decl_iter++) 
        {
            if (isa<CXXRecordDecl>(*decl_iter)) 
            {
                CXXRecordDecl *decl = static_cast<CXXRecordDecl *>(*decl_iter);

                TagTypeKind decl_type = decl->getTagKind();
                StringRef decl_name = decl->getName();

                if (decl_type != TTK_Struct)
                    continue;

                if (decl_name.startswith("__refflag_") && decl_name != class_name)
                {
                    bRefExportFlag = true;
                    AddRefClass(class_name.str(), Declaration, false);
                    bExport = true;
                }

                if(decl->getAccess() != AS_private)
                    continue;

                //以下为老的使用宏进行反射的方式
                /*if (decl_name.startswith("__regist_") ||
                        decl_name.startswith("__export_") ||
                        decl_name.startswith("__field_") ||
                        decl_name.startswith("__arrfield_") ||
                        decl_name.startswith("__method_") ||
                        decl_name.startswith("__staticmethod_") ||
                        decl_name.startswith("__constructor_"))
                {
                    char szBuff[512] = { 0 };

                    if (bOldExport == false)
                    {
                        snprintf(szBuff, sizeof(szBuff), "\r\n//--------------------------------------- %s ---------------------------------------\r\n", class_name.str().c_str());
                            ReflectionString.append(szBuff);
                    }

                    snprintf(szBuff, sizeof(szBuff), "struct %s::%s  %s::%s;\r\n", class_name.str().c_str(), decl_name.str().c_str(), class_name.str().c_str(), decl_name.str().c_str());
                    ReflectionString.append(szBuff);
                    bExport = true;
                    bOldExport = true;
                }*/
            }
            else if (isa<CXXMethodDecl>(*decl_iter)) 
            {
                auto it = ClassesInfo.find(class_name.str());
                if (it == ClassesInfo.end())
                    continue;

                if (bRefExportFlag == false)
                    continue;
                bRefExportFlag = false;

                CXXMethodDecl *decl = static_cast<CXXMethodDecl *>(*decl_iter);

                FunctionDecl *func_decl = decl->getAsFunction();

                std::string strFunc = func_decl->getDeclName().getAsString();
                //StringRef reffuncName = strFunc;

                AddClassFunc(class_name.str(), Declaration, strFunc, decl->isStatic(), decl->isConst(), func_decl);
            } 
            else if (isa<FieldDecl>(*decl_iter)) {
              auto it = ClassesInfo.find(class_name.str());
              if (it == ClassesInfo.end())
                continue;

              FieldDecl *decl = static_cast<FieldDecl *>(*decl_iter);
              StringRef strMem = decl->getName();
              std::string strType = decl->getType().getAsString();

              if (bRefExportFlag == false)
                continue;
              bRefExportFlag = false;

              AddClassMember(class_name.str(), Declaration, strMem.str(), strType, false);
            }
            else if (isa<VarDecl>(*decl_iter))
            {
                auto it = ClassesInfo.find(class_name.str());
                if (it == ClassesInfo.end())
                    continue;
                VarDecl* decl = static_cast<VarDecl*>(*decl_iter);
                StringRef strMem = decl->getName();
                std::string strType = decl->getType().getAsString();

                if (bRefExportFlag == false)
                    continue;
                bRefExportFlag = false;

                AddClassMember(class_name.str(), Declaration, strMem.str(), strType, decl->isStaticDataMember());
            }
        }
        if (bExport)
        {
            #ifdef WIN32
            StringRef basename = file_name.substr(file_name.find_last_of('\\') + 1, file_name.size());
            #else
            StringRef basename = file_name.substr(file_name.find_last_of('/') + 1, file_name.size());
            #endif
            if (Reflection_HeaderKey.find(basename.str()) == Reflection_HeaderKey.end()) 
            {
                Reflection_HeaderKey.insert(basename.str());
                Reflection_Header.push_back(basename.str());
            }
        }
        
        return true;
    }

    bool VisitEnumDecl(EnumDecl *Declaration) {

        /*if (bExportEnum)
        {
            SmallString<256> file_name = getFileName(Declaration->getLocation(), Context->getSourceManager());
            sys::path::native(file_name);

            SmallString<256> SourceDir(SwordGameHome);
            sys::path::append(SourceDir, "\\Source\\Server");

            std::string decl_name = Declaration->getName().str();
            if (file_name.startswith(SourceDir) && !(decl_name.empty() || decl_name == "Type"))
            {
                std::vector<name_val_line> vals;
                for (const EnumConstantDecl* ECD : Declaration->enumerators())
                {
                    vals.push_back(name_val_line(ECD->getName().str(), ECD->getInitVal()));
                }

                auto EnumDefInfo = std::make_pair(decl_name, vals);
                EnumDefines.push_back(EnumDefInfo);

            }
        }
        */
        return true;
    }

private:
    ASTContext *Context;
};

class ReflectionStructConsumer : public clang::ASTConsumer {
public:
    explicit ReflectionStructConsumer(ASTContext *Context)
        : Visitor(Context) {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }
private:
    ReflectionStructVisitor Visitor;
};

class ReflectionStructCSVisitAction : public clang::FrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
        return std::unique_ptr<clang::ASTConsumer>(
            new ReflectionStructConsumer(&Compiler.getASTContext()));
    }

    virtual void ExecuteAction() override;

    bool usesPreprocessorOnly() const override { return false; }

private:

};

void ReflectionStructCSVisitAction::ExecuteAction()
{
    CompilerInstance &CI = getCompilerInstance();
    if (!CI.hasPreprocessor())
        return;

    // FIXME: Move the truncation aspect of this into Sema, we delayed this till
    // here so the source manager would be initialized.
    if (hasCodeCompletionSupport() &&
        !CI.getFrontendOpts().CodeCompletionAt.FileName.empty())
        CI.createCodeCompletionConsumer();

    // Use a code completion consumer?
    CodeCompleteConsumer *CompletionConsumer = nullptr;
    if (CI.hasCodeCompletionConsumer())
        CompletionConsumer = &CI.getCodeCompletionConsumer();

    if (!CI.hasSema())
        CI.createSema(getTranslationUnitKind(), CompletionConsumer);

    ParseAST(CI.getSema(), CI.getFrontendOpts().ShowStats,
        CI.getFrontendOpts().SkipFunctionBodies);
}

bool SaveIfDifferent(StringRef file_path, std::string OutPutString)
{
    if (sys::fs::exists(file_path))
    {
        ErrorOr<std::unique_ptr<MemoryBuffer>> ContentOrErr =
            MemoryBuffer::getFileOrSTDIN(file_path);
        if (std::error_code EC = ContentOrErr.getError()) {
            errs() << EC.message() << "\n";
            return false;
        }
        std::unique_ptr<llvm::MemoryBuffer> Content = std::move(ContentOrErr.get());
        if (Content->getBufferSize() != 0 && Content->getBuffer().equals(OutPutString.c_str()))
        {
            return true;
        }
        sys::fs::remove(file_path);
    }
    
    std::error_code EC;
    raw_fd_ostream OS(file_path, EC, sys::fs::FA_Write);

    if (EC)
    {
        errs() << EC.message() << "\n";
        return false;
    }
    OS.write(OutPutString.c_str(), OutPutString.size());
    OS.close();
    return true;
}

bool CheckIsUpdateData(SmallString<256>& file_path)
{
    if (!sys::fs::exists(file_path))
        return false;

    ErrorOr<std::unique_ptr<MemoryBuffer>> ContentOrErr = MemoryBuffer::getFileOrSTDIN(file_path);
    if (std::error_code EC = ContentOrErr.getError())
        return false;

    std::unique_ptr<llvm::MemoryBuffer> Content = std::move(ContentOrErr.get());
    StringRef SourceData = Content->getBuffer();

    SmallVector<StringRef, 12> Headers;
    SplitString(SourceData, Headers, "\r\n");

    SmallString<256> code_path = sys::path::parent_path(sys::path::parent_path(file_path));

    int PrefixLen = strlen("#include \"");
    char szTimeBuff[64] = { 0 };
    std::string DBData;
    for (auto Line : Headers)
    {
        StringRef Header = Line.substr(PrefixLen, Line.size() - PrefixLen - 1);

        sys::fs::file_status FileStatus;
        if (auto EC = sys::fs::status(code_path + "/" + Header, FileStatus))
            return false;
        snprintf(szTimeBuff, sizeof(szTimeBuff), "%" PRId64, sys::toTimeT(FileStatus.getLastModificationTime()));

        DBData.append(Header.str());
        DBData.append("\t");
        DBData.append(szTimeBuff);
        DBData.append("\r\n");
    }

    SmallString<256> db_path = file_path;
    db_path.append(DBFileName);

    std::unique_ptr<llvm::MemoryBuffer> DBContent;
    if (sys::fs::exists(db_path))
    {
        ErrorOr<std::unique_ptr<MemoryBuffer>> DBContentOrErr = MemoryBuffer::getFileOrSTDIN(db_path);
        if (std::error_code EC = DBContentOrErr.getError())
            return false;

        DBContent = std::move(DBContentOrErr.get());
        if (DBContent->getBufferSize() != 0 && DBContent->getBuffer().equals(DBData))
        {
            return true;
        }
    }

    std::error_code EC;
    raw_fd_ostream OS(db_path, EC, sys::fs::FA_Write);

    if (EC)
    {
        return false;
    }

    OS.write(DBData.c_str(), DBData.size());
    OS.close();

    return false;
}

int main(int argc, char **argv) 
{
    std::vector<std::string> clang_args;
    SmallString<256> szSource(argv[1]);
    SmallString<256> szDest(argv[2]);
    SmallString<256> szArgs(argv[3]);

    std::vector<std::string> namespaceList;
    for (int i = 4; i < argc; i++)
    {
      std::string strArg = argv[i];
      namespaceList.push_back(strArg);
    }

    if (CheckIsUpdateData(szSource))
    {
        printf("Reflection code is up to date");
        return 0;
    }

    time_t tm_now = time(NULL);

    clang_args.push_back("-Wno-inconsistent-missing-override");
    clang_args.push_back("-Wno-deprecated-declarations");
    clang_args.push_back("-Wno-macro-redefined");
    clang_args.push_back("-Wno-gnu-folding-constant");
    clang_args.push_back("-DREF_EXPORT_FLAG");
    
    ErrorOr<std::unique_ptr<MemoryBuffer>> ArgsOrErr = MemoryBuffer::getFileOrSTDIN(szArgs);
    if (std::error_code EC = ArgsOrErr.getError())
    {
        errs() << EC.message() << "\n";
        return 1;
    }
    std::unique_ptr<llvm::MemoryBuffer> ArgsBuffer = std::move(ArgsOrErr.get());
    if (ArgsBuffer->getBufferSize() == 0)
        return 2;

    SmallVector<StringRef, 256> args;
    ArgsBuffer->getBuffer().split(args, "\n");
    for (auto arg : args)
    {
        std::string strParam = arg.str();
        strParam.erase(strParam.find_last_not_of("\r") + 1); 
        clang_args.push_back(strParam);
    }
    std::string strClangHeaderArg = "-I./";
    clang_args.push_back(strClangHeaderArg);

    ErrorOr<std::unique_ptr<MemoryBuffer>> CodeOrErr = MemoryBuffer::getFileOrSTDIN(szSource);
    if (std::error_code EC = CodeOrErr.getError())
    {
        errs() << EC.message() << "\n";
        return 1;
    }
    std::unique_ptr<llvm::MemoryBuffer> Code = std::move(CodeOrErr.get());
    if (Code->getBufferSize() == 0)
        return 2;

    Reflection_HeaderKey.clear();

    std::unique_ptr<FrontendAction> Action(new ReflectionStructCSVisitAction);
    clang::tooling::runToolOnCodeWithArgs(std::move(Action), Code->getBuffer(), clang_args);

    if (Reflection_HeaderKey.find("RefelectionHelper.h") == Reflection_HeaderKey.end())
    {
        Reflection_Header.push_back("RefelectionHelper.h");
    }

    std::string header;
    header.append("// C++ class header boilerplate exported from ReflectionTool\n// This is automatically generated by the tools.\n// DO NOT modify this manually!\n\n");
    for (auto itr : Reflection_Header)
    {
        header.append("#include \"");
        header.append(itr.c_str());
        header.append("\"\n");
    }

    for (size_t i = 0, isize = namespaceList.size(); i < isize; i++)
    {
      std::string strNameSpace = "\nusing namespace ";
      strNameSpace.append(namespaceList[i]);
      strNameSpace.append(";\n");
      header.append(strNameSpace);
    }

    std::string strRefContent;
    strRefContent.append(header);
    for (size_t i = 0, isize = ClassesVec.size(); i < isize; i++) {
      ExportClass2Ref(*ClassesVec[i], strRefContent);
      strRefContent.append("\n");
    }
    strRefContent.append(ReflectionString);

    if (!SaveIfDifferent(szDest, strRefContent)) {
      return 3;
    }

    time_t tm_end = time(NULL);

    printf("Reflection code generated in %" PRId64 " seconds", tm_end - tm_now);

    return 0;
}
