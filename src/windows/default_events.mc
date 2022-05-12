SeverityNames=(
  Success=0x0:STUMPLESS_WEL_SEVERITY_SUCCESS
  Informational=0x1:STUMPLESS_WEL_SEVERITY_INFORMATIONAL
  Warning=0x2:STUMPLESS_WEL_SEVERITY_WARNING
  Error=0x3:STUMPLESS_WEL_SEVERITY_ERROR
)

FacilityNames=(
  Kernel=0x100:STUMPLESS_WEL_FACILITY_KERN
  User=0x101:STUMPLESS_WEL_FACILITY_USER
  Mail=0x102:STUMPLESS_WEL_FACILITY_MAIL
  Daemon=0x103:STUMPLESS_WEL_FACILITY_DAEMON
  LogAuth=0x104:STUMPLESS_WEL_FACILITY_LOG_AUTH
  Syslog=0x105:STUMPLESS_WEL_FACILITY_SYSLOG
  Lpr=0x106:STUMPLESS_WEL_FACILITY_LPR
  News=0x107:STUMPLESS_WEL_FACILITY_NEWS
  Uucp=0x108:STUMPLESS_WEL_FACILITY_UUCP
  Cron=0x109:STUMPLESS_WEL_FACILITY_CRON
  Auth2=0x10a:STUMPLESS_WEL_FACILITY_AUTH2
  Ftp=0x10b:STUMPLESS_WEL_FACILITY_FTP
  Ntp=0x10c:STUMPLESS_WEL_FACILITY_NTP
  Audit=0x10d:STUMPLESS_WEL_FACILITY_AUDIT
  Alert=0x10e:STUMPLESS_WEL_FACILITY_ALERT
  Cron2=0x10f:STUMPLESS_WEL_FACILITY_CRON2
  Local0=0x110:STUMPLESS_WEL_FACILITY_LOCAL0
  Local1=0x111:STUMPLESS_WEL_FACILITY_LOCAL1
  Local2=0x112:STUMPLESS_WEL_FACILITY_LOCAL2
  Local3=0x113:STUMPLESS_WEL_FACILITY_LOCAL3
  Local4=0x114:STUMPLESS_WEL_FACILITY_LOCAL4
  Local5=0x115:STUMPLESS_WEL_FACILITY_LOCAL5
  Local6=0x116:STUMPLESS_WEL_FACILITY_LOCAL6
  Local7=0x117:STUMPLESS_WEL_FACILITY_LOCAL7
)

LanguageNames=(English=0x0409:MSG00409)

; // category definitions

MessageIdTypedef=WORD

MessageId=0x1
SymbolicName=STUMPLESS_EMERGENCY_EVENT
Language=English
Stumpless Emergency Event
.

MessageId=0x2
SymbolicName=STUMPLESS_ALERT_EVENT
Language=English
Stumpless Alert Event
.

MessageId=0x3
SymbolicName=STUMPLESS_CRITICAL_EVENT
Language=English
Stumpless Critical Event
.

MessageId=0x4
SymbolicName=STUMPLESS_ERR_EVENT
Language=English
Stumpless Error Event
.

MessageId=0x5
SymbolicName=STUMPLESS_WARNING_EVENT
Language=English
Stumpless Warning Event
.

MessageId=0x6
SymbolicName=STUMPLESS_NOTICE_EVENT
Language=English
Stumpless Notice Event
.

MessageId=0x7
SymbolicName=STUMPLESS_INFO_EVENT
Language=English
Stumpless Informational Event
.

MessageId=0x8
SymbolicName=STUMPLESS_DEBUG_EVENT
Language=English
Stumpless Debug Event
.

; // message definitions

MessageIdTypedef=DWORD

MessageId=0x11
Severity=Success
Facility=Kernel
SymbolicName=STUMPLESS_MSG_SUCCESS_KERNEL
Language=English
Stumpless Kernel Success message.
.

MessageId=0x17
Severity=Success
Facility=User
SymbolicName=STUMPLESS_MSG_SUCCESS_USER
Language=English
Stumpless User Success message.
.
