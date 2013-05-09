!include mo-config.nsh

!define AppName               "Firefox"
!define AppVersion            "latest"
!define GREVersion            ${AppVersion}

!define FileMainEXE           "firefox.exe"
!define WindowClass           "FirefoxMessageWindow"
!define DDEApplication        "Firefox"
!define AppRegName            "Firefox"

!define BrandShortName        "Firefox"
!define PreReleaseSuffix      ""
!define BrandFullName         "${BrandFullNameInternal}${PreReleaseSuffix}"
!define MOInstallerVersion    "3.0.3"

!define NO_UNINSTALL_SURVEY

!define LSP_CATEGORIES "0x00000000"

!define ARCH "x86"
!define MinSupportedVer "Microsoft Windows XP SP2"

!define MOZ_MAINTENANCE_SERVICE

VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName"     "${BrandShortName}"
VIAddVersionKey "CompanyName"     "${CompanyName}"
VIAddVersionKey "LegalTrademarks" "${BrandShortName} is a Trademark of The Mozilla Foundation."
VIAddVersionKey "LegalCopyright"  "${CompanyName}"
VIAddVersionKey "FileVersion"     "${AppVersion}"
VIAddVersionKey "ProductVersion"  "${AppVersion}"

!define APPROXIMATE_REQUIRED_SPACE_MB "42.2"

!define OPTIONS_ITEM_EDGE_DU 90u
!define OPTIONS_ITEM_WIDTH_DU 356u
!define OPTIONS_SUBITEM_EDGE_DU 119u
!define OPTIONS_SUBITEM_WIDTH_DU 327u
!define INSTALL_BLURB_TOP_DU 78u
!define APPNAME_BMP_EDGE_DU 19u
!define APPNAME_BMP_TOP_DU 12u
