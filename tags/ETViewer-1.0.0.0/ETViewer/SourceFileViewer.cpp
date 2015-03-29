//  ETViewer, an easy to use ETW / WPP trace viewer
//  Copyright (C) 2011  Javier Martin Garcia (javiermartingarcia@gmail.com)
//  
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//  
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
////////////////////////////////////////////////////////////////////////////////////////
//
// Project HomePage: http://etviewer.codeplex.com/
// For any comment or question, mail to: etviewer@gmail.com
//
////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ETViewer.h"
#include "SourceFileViewer.h"
#include "SourceFileContainer.h"

#define TAB_SIZE 4

TCHAR *g_pKeywords[]={
                        /*C++ Keywords*/			_T("asm1"),_T("auto"),_T("bad_cast"),_T("bad_typeid"),_T("bool"),_T("break"),_T("case"),_T("catch"),_T("TCHAR"),_T("class"),_T("const"),_T("const_cast"),_T("continue"),_T("default"),_T("delete"),_T("do"),_T("double"),_T("dynamic_cast"),_T("else"),_T("enum"),_T("except"),_T("explicit"),_T("extern"),_T("false"),_T("finally"),_T("float"),_T("for"),_T("friend"),_T("goto"),_T("if"),_T("inline"),_T("int"),_T("long"),_T("mutable"),_T("namespace"),_T("new"),_T("operator"),_T("private"),_T("protected"),_T("public"),_T("register"),_T("reinterpret_cast"),_T("return"),_T("short"),_T("signed"),_T("sizeof"),_T("static"),_T("static_cast"),_T("struct"),_T("switch"),_T("template"),_T("this"),_T("throw"),_T("true"),_T("try"),_T("type_info"),_T("typedef"),_T("typeid"),_T("typename"),_T("union"),_T("unsigned"),_T("using"),_T("virtual"),_T("void"),_T("volatile"),_T("while"),
                        /*Preprocessor Directives*/	_T("#define"),_T("#error"),_T("#import"),_T("#undef"),_T("#elif"),_T("#if"),_T("#include"),_T("#else"),_T("#ifdef"),_T("#line"),_T("#endif"),_T("#ifndef"),_T("#pragma"),
                                                    _T("BOOL"),_T("HANDLE"),_T("DWORD"),_T("FALSE"),_T("TRUE"),_T("boolean"),_T("LRESULT"),_T("hyper"),_T("BYTE"),_T("LONG"),_T("ULONG"),_T("UINT"),_T("UIN16"),_T("set"),_T("map"),_T("list"),_T("deque"),_T("vector"),_T("BEGIN_DBTABLE"),_T("DBTABLE_COMP"),_T("END_DBTABLE"),_T("BEGIN_COM_MAP"),_T("COM_INTERFACE_ENTRY"),_T("COM_INTERFACE_ENTRY_CHAIN"),_T("END_COM_MAP"),_T("DECLARE_GUIOBJECT"),_T("DECLARE_PPAGE"),_T("DECLARE_COMPOSITE"),_T("BEGIN_MSG_MAP"),_T("CHAIN_MSG_MAP"),_T("MESSAGE_HANDLER"),_T("COMMAND_HANDLER"),
                                                    _T("COMMAND_ID_HANDLER"),_T("COMMAND_CODE_HANDLER"),_T("COMMAND_RANGE_HANDLER"),_T("NOTIFY_HANDLER"),_T("NOTIFY_ID_HANDLER"),_T("NOTIFY_CODE_HANDLER"),_T("NOTIFY_RANGE_HANDLER"),_T("END_MSG_MAP"),_T("VAA2W"),_T("VAW2A"),_T("_CSA2W"),_T("_GRS"),_T("LS"),_T("NULL"),_T("CLSID_NULL"),_T("GUID_NULL"),_T("IID_NULL"),_T("SUCCEEDED"),_T("FAILED"),_T("VA_SLAVE_DB"),_T("VA_MASTER_DB"),
                        NULL};




TCHAR *g_Separators={_T(" ,.:;'\\\"+-*/%=!?¿<>[](){}\t\n\r&|~^")};

/////////////////////////////////////////////////////////////////////////////
// CSourceFileViewer dialog

void SetRichEditTextColor(CRichEditCtrl *pEdit,DWORD begin,DWORD end,COLORREF color)
{
    CHARFORMAT format={0};
    format.cbSize=sizeof(format);
    format.crTextColor=color;
    format.dwMask=CFM_COLOR;
    format.dwEffects&=~CFE_AUTOCOLOR;

    pEdit->SetSel(begin,end);
    pEdit->SendMessage(EM_SETCHARFORMAT,SCF_SELECTION,(DWORD)&format);
    pEdit->SetSel(0,0);
}

CSourceFileViewer::CSourceFileViewer(CSourceFileContainer* pParent /*=NULL*/)
    : CDialog(CSourceFileViewer::IDD, pParent)
{

    //{{AFX_DATA_INIT(CSourceFileViewer)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    LoadLibrary(_T("RICHED32.DLL"));

    m_bHideTracingOptions=true;
    m_bHideDeleteButtons=true;
    m_bHideMarkButtons=true;
    m_pContainer=pParent;
    
    m_pFileBufferUpper=NULL;
    m_pFileBuffer=NULL;
    m_FileBufferLength=0;
    m_hFileFont=NULL;
    m_OldEditProc=0;
    m_SourceFile[0]=0;
    m_SourceLine=0;
}


void CSourceFileViewer::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSourceFileViewer)
    DDX_Control(pDX, IDC_ED_LINE, m_EDLine);
    DDX_Control(pDX, IDC_ED_FULL_PATH, m_EDFullPath);
    DDX_Control(pDX, IDC_ED_FILE, m_EDFile);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSourceFileViewer, CDialog)
    //{{AFX_MSG_MAP(CSourceFileViewer)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDCANCEL, OnCancel)
    ON_BN_CLICKED(IDOK, OnOK)
//	ON_MESSAGE(WM_USER+1,OnUpdateSelectedLine)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSourceFileViewer message handlers

void CSourceFileViewer::OnOK() 
{
}

void CSourceFileViewer::OnCancel() 
{
}

DWORD CSourceFileViewer::OpenFile(const TCHAR *pFile,int line,bool bShowErrorIfFailed)
{
    delete [] m_pFileBufferUpper;
    delete [] m_pFileBuffer;
    m_pFileBufferUpper=NULL;
    m_pFileBuffer=NULL;

    DWORD result=0;
    _tcscpy_s(m_SourceFile,MAX_PATH,pFile);
    m_SourceLine=line;

    HANDLE hFile=CreateFile(m_SourceFile,GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile==INVALID_HANDLE_VALUE)
    {
        result=GetLastError();

        TCHAR sTemp[1024];
        _stprintf_s(sTemp,1024,_T("Cannot open source file %s due to error %d"),m_SourceFile,result);
        if(bShowErrorIfFailed){MessageBox(sTemp,_T(""),MB_ICONSTOP|MB_OK);}
    }
    else
    {
        theApp.AddRecentSourceFile(m_SourceFile);

        DWORD bytesRead=0;
        m_FileBufferLength=GetFileSize(hFile,NULL);

        TCHAR  *pReadedBuffer=new TCHAR[m_FileBufferLength+1000];
        DWORD *pKeywordLenght=new DWORD [1000];
        m_pFileBuffer=new TCHAR [m_FileBufferLength*TAB_SIZE+100];
        m_pFileBufferUpper=new TCHAR [m_FileBufferLength*TAB_SIZE+100];
        if(m_pFileBuffer && m_pFileBufferUpper)
        { 
            unsigned x=0,y=0;
            while(g_pKeywords[x]!=NULL){pKeywordLenght[x]=(int)_tcslen(g_pKeywords[x]);x++;}

            if(ReadFile(hFile,pReadedBuffer,m_FileBufferLength,&bytesRead,NULL))
            {
                unsigned lastLineIndex=0;
                m_FileBufferLength=0;
                for(x=0;x<bytesRead;x++)
                {

                    if(pReadedBuffer[x]==_T('\t'))
                    {
                        int tabsToAdd=TAB_SIZE-((m_FileBufferLength-lastLineIndex)%4);

                        memset(m_pFileBuffer+m_FileBufferLength,' ',tabsToAdd);
                        m_FileBufferLength+=tabsToAdd;
                    }
                    else if(pReadedBuffer[x]!=_T('\r'))
                    {
                        m_pFileBuffer[m_FileBufferLength]=pReadedBuffer[x];m_FileBufferLength++;
                    }

                    if(pReadedBuffer[x]==_T('\n'))
                    {
                        lastLineIndex=m_FileBufferLength;
                    }
                }
                m_pFileBuffer[m_FileBufferLength]=0;
                _tcscpy_s(m_pFileBufferUpper,m_FileBufferLength*TAB_SIZE+100, m_pFileBuffer);
                _tcsupr_s(m_pFileBufferUpper, m_FileBufferLength*TAB_SIZE+100);

                m_EDFile.SetWindowText(m_pFileBuffer);
                SetRichEditTextColor(&m_EDFile,0,-1,RGB(0,0,0));


                for(x=0;x<m_FileBufferLength;x++)
                {
                    TCHAR *pToken=m_pFileBuffer+x;
                    if(_tcschr(g_Separators,m_pFileBuffer[x])==NULL && (x==0 || _tcschr(g_Separators,m_pFileBuffer[x-1])!=NULL))
                    {
                        DWORD offset=(DWORD)(pToken-m_pFileBuffer);
                        for(y=0;g_pKeywords[y]!=NULL;y++)
                        {
                            if(m_pFileBuffer[x]==g_pKeywords[y][0])
                            {
                                DWORD keyLen=pKeywordLenght[y];
                                if((offset+keyLen)<=m_FileBufferLength)
                                {
                                    if(_tcsncmp(pToken,g_pKeywords[y],keyLen)==0 && ((offset+keyLen)==(m_FileBufferLength) ||_tcschr(g_Separators,pToken[keyLen])!=NULL))
                                    {
                                        int pos=pToken-m_pFileBuffer;
                                        SetRichEditTextColor(&m_EDFile,pos,pos+keyLen,RGB(0,0,255));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if(m_pFileBuffer[x]==_T('"'))
                    {
                        TCHAR *pBase=m_pFileBuffer+x;
                        x++;
                        while(x<m_FileBufferLength)
                        {
                            if(m_pFileBuffer[x]==_T('"') && (x==0 || m_pFileBuffer[x-1]!=_T('\\')))
                            {
                                x++;
                                break;
                            }
                            x++;
                        }
                    }
                    else
                    {
                        TCHAR *pBase=m_pFileBuffer+x;
                        if(m_pFileBuffer[x]==_T('/') && m_pFileBuffer[x+1]==_T('*'))
                        {
                            x+=2;
                            while(x<m_FileBufferLength)
                            {
                                if(m_pFileBuffer[x]==_T('*') && m_pFileBuffer[x+1]==_T('/'))
                                {
                                    x+=2;
                                    break;
                                }
                                x++;
                            }
                            int pos=pBase-m_pFileBuffer;
                            SetRichEditTextColor(&m_EDFile,pos,x,RGB(0,128,0));
                        }
                        if(m_pFileBuffer[x]==_T('/') && m_pFileBuffer[x+1]==_T('/'))
                        {
                            x+=2;
                            while(x<m_FileBufferLength)
                            {
                                if(m_pFileBuffer[x]==_T('\n'))
                                {
                                    break;
                                }
                                x++;
                            }
                            int pos=pBase-m_pFileBuffer;
                            SetRichEditTextColor(&m_EDFile,pos,x,RGB(0,128,0));
                        }
                    }
                }

                m_EDFile.SetSel(0,0);
            }
        }
        delete [] pReadedBuffer;
        delete [] pKeywordLenght;
        if(hFile){CloseHandle(hFile);hFile=NULL;}
    }

    SetMetrics();
    UpdateLine();
    PostMessage(WM_USER+1);
    ShowLine(m_SourceLine);

    std::tstring sTemp;
    sTemp=m_SourceFile;
    if(m_SourceLine)
    {
        TCHAR sTemp2[1024];
        _stprintf_s(sTemp2,1024,_T(" (%d)"),m_SourceLine);
        sTemp+=sTemp2;
    }
    SetWindowText(sTemp.c_str());
    m_EDFullPath.SetWindowText(m_SourceFile);

    return result;
} 


void CSourceFileViewer::Reload()
{
    OpenFile(m_SourceFile,m_SourceLine,false);
}

BOOL CSourceFileViewer::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    LOGFONT logFont = {0};
    logFont.lfHeight = 10;
    StringCbPrintf(logFont.lfFaceName,sizeof(logFont.lfFaceName),_T("Courier"));

    m_hFileFont=CreateFontIndirect(&logFont);
    m_EDFile.SendMessage(WM_SETFONT,(DWORD)m_hFileFont,true);

    m_OldEditProc=GetWindowLong(m_EDFile.m_hWnd,GWL_WNDPROC);
    SetWindowLong(m_EDFile.m_hWnd,GWL_WNDPROC,(DWORD)FileEditProc);
    SetWindowLong(m_EDFile.m_hWnd,GWL_USERDATA,(DWORD)this);
    m_hFindOwner=m_EDFile.m_hWnd;
    return FALSE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CSourceFileViewer::OnDestroy() 
{
    if(m_pFindDialog){m_pFindDialog->m_pFindClient=NULL;}

    CDialog::OnDestroy();


    delete [] m_pFileBufferUpper;
    delete [] m_pFileBuffer;
    
    if(m_hFileFont){DeleteObject((HGDIOBJ)m_hFileFont);}
}

void CSourceFileViewer::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize(nType, cx, cy);
    if(m_EDFile.m_hWnd){SetMetrics();}
}


void CSourceFileViewer::SetMetrics()
{
    RECT clientRect,fileRect,pathRect,lineRect;
    GetClientRect(&clientRect);
    m_EDFile.GetWindowRect(&fileRect);
    m_EDFullPath.GetWindowRect(&pathRect);
    m_EDLine.GetWindowRect(&lineRect);

    SIZE clientSize=GetRectSize(clientRect);
    SIZE fileSize=GetRectSize(fileRect);
    SIZE pathSize=GetRectSize(pathRect);
    SIZE lineSize=GetRectSize(lineRect);
    POINT filePos,pathPos,linePos;

    filePos.x=0;
    filePos.y=0;
    fileSize.cx=clientSize.cx;
    fileSize.cy=clientSize.cy-(pathSize.cy+2);
    
    pathPos.x=0;
    pathPos.y=clientSize.cy-pathSize.cy;
    pathSize.cx=clientSize.cx-(lineSize.cx+1);

    linePos.x=clientSize.cx-lineSize.cx;
    linePos.y=clientSize.cy-lineSize.cy;
    m_EDFile.SetWindowPos(NULL,filePos.x,filePos.y,fileSize.cx,fileSize.cy,SWP_NOZORDER);
    m_EDFullPath.SetWindowPos(NULL,pathPos.x,pathPos.y,pathSize.cx,pathSize.cy,SWP_NOZORDER);
    m_EDLine.SetWindowPos(NULL,linePos.x,linePos.y,lineSize.cx,lineSize.cy,SWP_NOZORDER);
}

void CSourceFileViewer::ShowLine(int line)
{
    m_SourceLine=line;
    OnUpdateSelectedLine();
    UpdateLine();
}

void CSourceFileViewer::OnUpdateSelectedLine()
{
    m_EDFile.SetSel(0,0);
    if(m_SourceLine)
    {
        int lineToFocus=m_SourceLine-2;
        int firstChar=m_EDFile.LineIndex(m_SourceLine-1);
        int lastChar=m_EDFile.LineIndex(m_SourceLine);
        m_EDFile.SetSel(firstChar,lastChar);
        m_EDFile.LineScroll(-m_EDFile.GetLineCount(),0);
        m_EDFile.LineScroll(lineToFocus-1);
    }

}

void CSourceFileViewer::UpdateLine()
{
    long selBegin=0,selEnd=0,line=0,col=0;
    m_EDFile.GetSel(selBegin,selEnd);
    line=m_EDFile.LineFromChar(selBegin);
    col=selBegin-m_EDFile.LineIndex(line);
    TCHAR sTemp[1024];
    _stprintf_s(sTemp,1024,_T("Ln %d,Col %d"),line+1,col+1);
    m_EDLine.SetWindowText(sTemp);
}

void CSourceFileViewer::OnFind()
{
    ShowFindDialog();
}

void CSourceFileViewer::ShowFindDialog()
{
    long selBegin=0,selEnd=0,line=0,col=0;
    m_EDFile.GetSel(selBegin,selEnd);
    CString sSelText = m_EDFile.GetSelText();
    m_LastTextToFind=sSelText.GetBuffer();
    BeginFind(this,m_EDFile.m_hWnd,m_LastTextToFind.c_str());
    sSelText.ReleaseBuffer();
}


LRESULT CALLBACK CSourceFileViewer::FileEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CSourceFileViewer *pThis=(CSourceFileViewer *)GetWindowLong(hwnd,GWL_USERDATA);
    if(uMsg==WM_RBUTTONDOWN)
    {
        long begin=0,end=0;
        TCHAR A[MAX_PATH]={0};
        TCHAR fileToOpen[MAX_PATH]={0};
        pThis->m_EDFile.GetSel(begin,end);
        POINTL P={LOWORD(lParam),HIWORD(lParam)};
        int TCHARacter=pThis->m_EDFile.SendMessage(EM_CHARFROMPOS,0,(DWORD)&P);

        if(TCHARacter<begin || TCHARacter>end || begin==end || begin>end)
        {
            begin=end=TCHARacter;
            int line=pThis->m_EDFile.LineFromChar(TCHARacter);
            if(line>=0)
            {
                long TCHARIndex=TCHARacter-pThis->m_EDFile.LineIndex(line);
                pThis->m_EDFile.GetLine(line,A,_countof(A)-1);
                if(TCHARIndex>=(long)_tcslen(A)){TCHARIndex=0;}

                bool includebegin=0,includeend=0;
                TCHAR beginCharToFind=_T('"');
                TCHAR *pBegin=NULL,*pEnd=_tcschr(A+TCHARIndex,_T('"'));
                if(pEnd==NULL){pEnd=_tcschr(A+TCHARIndex,_T('>'));beginCharToFind=_T('<');}
                if(pEnd!=NULL)
                {
                    pEnd[0]=0;
                    pBegin=_tcsrchr(A,beginCharToFind);
                    if(pBegin)
                    {
                        pBegin++;//skip " TCHARacter
                        (*pEnd)=0;
                        _tcsncpy_s(fileToOpen,MAX_PATH,pBegin,_countof(fileToOpen)-1);
                    }
                }
            }

        }
        else
        {
            CString sSelText = pThis->m_EDFile.GetSelText();
            _tcsncpy_s(fileToOpen,MAX_PATH,sSelText,_countof(fileToOpen)-1);
        }
        if(fileToOpen[0]!=0)
        {
            POINT p={0,0};
            ::GetCursorPos(&p);
            HMENU hMenu=GetSubMenu(LoadMenu(AfxGetResourceHandle(),MAKEINTRESOURCE(IDM_FILE_VIEWER)),0);

            TCHAR A[2000];
            _stprintf_s(A,2000,_T("Open '%s'"),fileToOpen);
            MENUITEMINFO itemInfo={0};
            itemInfo.cbSize=sizeof(itemInfo);
            itemInfo.cch=(unsigned)_tcslen(A);
            itemInfo.dwTypeData=A;
            itemInfo.fMask=MIIM_STRING;

            SetMenuItemInfo(hMenu,ID_OPEN_AS_SOURCE_FILE,MF_BYCOMMAND,&itemInfo);
            DWORD command=::TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RETURNCMD,p.x,p.y,0,pThis->m_hWnd,NULL);
            if(command==ID_OPEN_AS_SOURCE_FILE)
            {
                std::deque<std::tstring> dirs;

                // include the same directory as the file showed by this viewer
                std::tstring temp1;
                TCHAR drive[MAX_PATH]={0},path[MAX_PATH]={0};
                _tsplitpath_s(pThis->m_SourceFile, drive, MAX_PATH, path, MAX_PATH, NULL, 0, NULL, 0);
                temp1=drive;
                temp1+=path;
                if(_tcscmp(temp1.c_str(),_T(""))!=0){dirs.push_back(temp1);}

                int x=0;
                for(x=0;x<(int)theApp.m_SourceDirectories.size();x++)
                {
                    dirs.push_back(theApp.m_SourceDirectories[x].c_str());
                }
                if(!theApp.OpenCodeAddress(fileToOpen,0,false))
                {
                    for(x=0;x<(int)dirs.size();x++)
                    {
                        TCHAR fullPath[MAX_PATH]={0};
                        bool noFinalSlash=true;
                        _tcsncpy_s(fullPath,MAX_PATH,dirs[x].c_str(),_countof(fullPath)-2); //-2 for the additional slash
                        DWORD len=(DWORD)_tcslen(fullPath);
                        if(len)
                        {
                            if(fullPath[len-1]!=_T('\\'))
                            {
                                fullPath[len]=_T('\\');
                                fullPath[len+1]=0;
                                len++;
                            }
                        }

                        _tcsncat_s(fullPath,fileToOpen[0]==_T('\\')?fileToOpen+1:fileToOpen,_countof(fullPath)-1);

                        if(theApp.OpenCodeAddress(fullPath,0,false)){break;}
                    }
                }		
            }
        }

        return 0;
    }

    if(uMsg==WM_SETFOCUS)
    {
        long begin=0,end=0;
        pThis->m_EDFile.GetSel(begin,end);
        LRESULT res=CallWindowProc((WNDPROC)pThis->m_OldEditProc,hwnd,uMsg,wParam,lParam);
        pThis->m_EDFile.SetSel(begin,end);
        return res;
    }

    if(uMsg==WM_KEYDOWN)
    {
        bool pushedLControl=(GetKeyState(VK_LCONTROL)>>15)?true:false;
        bool pushedRControl=(GetKeyState(VK_RCONTROL)>>15)?true:false;
        bool pushedLShift=(GetKeyState(VK_LSHIFT)>>15)?true:false;
        bool pushedRShift=(GetKeyState(VK_RSHIFT)>>15)?true:false;
        bool pushedControl=(pushedLControl||pushedRControl);
        bool pushedShift=(pushedLShift||pushedRShift);
        if(wParam=='F' && (pushedControl)){pThis->OnFind();return 0;}
        if(wParam==VK_TAB && (pushedControl))
        {
            if(pushedShift){pThis->m_pContainer->SelectPrevious();}else{pThis->m_pContainer->SelectNext();}
            return 0;
        }
        if(wParam==VK_NEXT && (pushedControl)){pThis->m_pContainer->SelectNext();return 0;}
        if(wParam==VK_PRIOR && (pushedControl)){pThis->m_pContainer->SelectPrevious();return 0;}

        if(wParam==VK_F3 && !(pushedControl))
        {
            bool dir=pThis->m_bFindDirectionUp;
            pThis->m_bFindDirectionUp=pushedShift;
            pThis->FindNext(pThis->m_LastTextToFind.c_str());
            pThis->m_bFindDirectionUp=dir;
            return 0;
        }
    
    }

    if((uMsg>=WM_KEYFIRST && uMsg<=WM_KEYLAST) || (uMsg>=WM_MOUSEFIRST && uMsg<=WM_MOUSELAST) && uMsg!=WM_MOUSEMOVE)
    {
        LRESULT res=CallWindowProc((WNDPROC)pThis->m_OldEditProc,hwnd,uMsg,wParam,lParam);
        pThis->UpdateLine();
        return res;
    }
    
    return CallWindowProc((WNDPROC)pThis->m_OldEditProc,hwnd,uMsg,wParam,lParam);
}

bool CSourceFileViewer::FindAndDeleteAll(const TCHAR *pText)
{
    return false;
}

bool CSourceFileViewer::FindAndMarkAll(const TCHAR *pText)
{
    return false;
}

TCHAR *strrstr(TCHAR *pBuffer,DWORD offset,TCHAR *pTextToFind)
{
    unsigned textToFindLenght=(unsigned)_tcslen(pTextToFind);
    for(int x=(offset-textToFindLenght);x>=0;x--)
    {
        if(memcmp(pBuffer+x,(TCHAR*)pTextToFind,textToFindLenght)==0)
        {
            return pBuffer+x;
        }
    }
    return NULL;
}


bool CSourceFileViewer::FindNext(const TCHAR *pTextToFind)
{
    if(m_pFileBuffer==NULL){return false;}

    long begin=0,end=0;
    int res=-1;
    m_EDFile.GetSel(begin,end);
    if(begin<0){begin=0;}

    TCHAR  textToFind[1024]={0};
    TCHAR *pText=NULL,*pBufferToSearchIn=m_bMatchCaseInFind?m_pFileBuffer:m_pFileBufferUpper;
    _tcscpy_s(textToFind,m_LastTextToFind.c_str());
    if(!m_bMatchCaseInFind){_tcsupr_s(textToFind, 1024);}
    unsigned textToFindLenght=(unsigned)_tcslen(textToFind);

    TCHAR *pTextFound=NULL;
    if(m_bFindDirectionUp)
    {
        pText=strrstr(pBufferToSearchIn,begin,textToFind);
        if(pText==NULL){pText=strrstr(pBufferToSearchIn,m_FileBufferLength-1,textToFind);}
        begin=pText-pBufferToSearchIn;
    }
    else
    {
         pText=_tcsstr(pBufferToSearchIn+end,(TCHAR*)textToFind);
         if(pText==NULL){pText=_tcsstr(pBufferToSearchIn,(TCHAR*)textToFind);}
         begin=pText-pBufferToSearchIn;
    }

    CWnd *pParent=GetActiveWindow();
    if(!pParent){pParent=this;}
    if(pText==NULL)
    {
        std::tstring sTemp;
        sTemp=m_LastTextToFind;
        sTemp+=_T(" was not found");
        pParent->MessageBox(sTemp.c_str(),_T(""),MB_OK);
    }
    if(pText){m_EDFile.SetSel(begin,begin+textToFindLenght);}
    return pText!=NULL;
}

void CSourceFileViewer::SetFocusOnOwnerWindow()
{
}

void CSourceFileViewer::Copy()
{
    m_EDFile.Copy();
}

std::tstring CSourceFileViewer::GetFile()
{
    return m_SourceFile;
}