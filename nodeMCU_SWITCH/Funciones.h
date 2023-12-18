#include <ESP_Mail_Client.h>

int ESP_Despierto = D2;
int Cantidad_Archivos;
bool Mando_Mail;
boolean Dormir = false;

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT esp_mail_smtp_port_465
SMTPSession smtp;
void smtpCallback(SMTP_Status status);
//==============================================================================
/* Configuraciones de red y correo electrónico del remitente */

#define WIFI_SSID     "MC116_02EB2"                 // nombre de la red a la cual conectarse
#define WIFI_PASSWORD   "5A6b7C8d"                  // contraseña de la red a la cual conectarse

#define AUTHOR_EMAIL "jcito087@gmail.com"           //cuenta del remitente
#define AUTHOR_PASSWORD "ybdgixwzrkzgllwu"          //contraseña de aplicacion de la cuenta del remitente

#define EMISOR "Caja GUT - noRTC"                             // Nombre generico del emisor del correo
#define ASUNTO "Datos de hoy"                       // Asunto del correo

#define RECEPTOR_NOMBRE "LSD wannabe"               // Nombre generico del receptor
#define RECEPTOR_MAIL   "datosinalambricoslsd@gmail.com"      // Mail del receptor
//==============================================================================
/* Configuraciones del mail */

#define MIME F("audio/wav")                         // tipo de archivo / su extensión [https://developer.mozilla.org/es/docs/Web/HTTP/Basics_of_HTTP/MIME_types]
//==============================================================================

void SendMail(String Nombre, String Dire) {

  MailClient.networkReconnect(true);

  smtp.debug(0);
  smtp.callback(smtpCallback);
  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = F("mydomain.net");     // Funciona sin tocarlo. Si hay que cambiar algo ver el ejemplo
  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  SMTP_Message message;
  message.enable.chunking = true;
  message.sender.name = EMISOR;
  message.sender.email = AUTHOR_EMAIL;
  message.subject = ASUNTO;
  message.addRecipient(RECEPTOR_NOMBRE, RECEPTOR_MAIL);

  SMTP_Attachment att[1];                           // Modificar este número según el número de archivos, y agregar un attIndex por cada uno, con sus propiedades

  int attIndex = 0;
  att[attIndex].descr.filename = Nombre;
  att[attIndex].descr.mime = MIME;
  att[attIndex].file.path = Dire;
  att[attIndex].file.storage_type = esp_mail_file_storage_type_sd;
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.addAttachment(att[attIndex]);

  if (!smtp.connect(&config))
    return;
  if (MailClient.sendMail(&smtp, &message, true)) {
    SD.remove(Nombre);
  }
}
//==============================================================================
void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
    }
    smtp.sendingResult.clear();
  }
}
//==============================================================================
void Lectura_Directorio(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    String Nombre = entry.name();
    delay(1000);
    if (digitalRead(ESP_Despierto) == 0) {
      break;
    }
    if (Nombre.endsWith(".WAV") || Nombre.endsWith(".wav")) {         // En esta línea se decide qué extensión de archivos tendrán los enviados
      String Dire = "/" + Nombre;
      SendMail(Nombre, Dire);
    }
    if (entry.isDirectory()) {
      Lectura_Directorio(entry, numTabs + 1);
    }
    entry.close();
  }
}
//=========================================================================================================
uint16_t Conteo_Archivos() {
  File root = SD.open("/");
  int numFiles = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    String Nombre = entry.name();
    if (Nombre.endsWith(".wav") || Nombre.endsWith(".WAV")) {
      numFiles++;
    }
    entry.close();
  }
  return numFiles;
}
