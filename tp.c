/* Compilador del Lenguaje Micro (Fischer)	*/
//Inclusión de bibliotecas con sus funciones
#include <stdio.h>
#include <string.h>
#include <ctype.h>


// Macros

#define NUMESTADOS 15
#define NUMCOLS 13
#define TAMLEX 32 + 1
#define TAMNOM 20 + 1

// El + 1 sirve para indicar que son 30 caracteres + el fin de texto (\0)
 
/******************Declaraciones Globales*************************/
FILE *in; //Se crea la variable del archivo con el código
typedef enum // Un tipo enum que se le agrega el alias "Token", para eso sirve el typedef (cambiar su forma de leerlo)
             // Sería como el type en haskell 
{
   INICIO,
   FIN,
   LEER,
   ESCRIBIR,
   ID,
   CONSTANTE,
   PARENIZQUIERDO,
   PARENDERECHO,
   PUNTOYCOMA,
   COMA,
   ASIGNACION,
   SUMA,
   RESTA,
   FDT,
   ERRORLEXICO
} TOKEN;

// FDT = Fin de texto = \0 o $.
 
typedef struct 
{
   char identifi[TAMLEX];
   TOKEN t; /* t=0, 1, 2, 3 Palabra Reservada, t=ID=4 Identificador */
} RegTS;
// Chat: Representa la tabla de simbolos, contiene info sobre 
// identificadores como su nombre tipo y posicion en programa.
// sirve para análisis semantico y generación de código.
 
RegTS TS[1000] = {{"inicio", INICIO}, {"fin", FIN}, {"leer", LEER}, {"escribir", ESCRIBIR}, {"$", ID}};

typedef struct
{
   TOKEN clase;
   char nombre[TAMLEX];
   int valor;
} REG_EXPRESION;
// Chat: Representa una expresión regular para el análisis léxico.

char buffer[TAMLEX]; //Cada instrucción en micro tiene un espacio total de 32 bits + fin de linea. Aca se "acumulan" todas las instrucciones.

TOKEN tokenActual;

int flagToken = 0;

/**********************Prototipos de Funciones************************/
TOKEN scanner();
int columna(int c);
int estadoFinal(int e);
void Objetivo(void);
void Programa(void);
void ListaSentencias(void);
void Sentencia(void);
void ListaIdentificadores(void);
void Identificador(REG_EXPRESION *presul);
void ListaExpresiones(void);
void Expresion(REG_EXPRESION *presul);
void Primaria(REG_EXPRESION *presul);
void OperadorAditivo(char *presul);

REG_EXPRESION ProcesarCte(void);
REG_EXPRESION ProcesarId(void);
char *ProcesarOp(void);
void Leer(REG_EXPRESION in);
void Escribir(REG_EXPRESION out);
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char *op, REG_EXPRESION e2);

void Match(TOKEN t);
TOKEN ProximoToken();
void ErrorLexico();
void ErrorSintactico();
void Generar(char *co, char *a, char *b, char *c);
char *Extraer(REG_EXPRESION *preg);
int Buscar(char *id, RegTS *TS, TOKEN *t);
void Colocar(char *id, RegTS *TS);
void Chequear(char *s);
void Comenzar(void);
void Terminar(void);
void Asignar(REG_EXPRESION izq, REG_EXPRESION der);

/***************************Programa Principal************************/
   // Verifica la cantidad de argumentos y la extensión del archivo fuente
   // Abre el archivo fuente y llama a la función de compilación
   // Cierra el archivo al terminar
int main(int argc, char *argv[])
{
   TOKEN tok;
   char nomArchi[TAMNOM];
   int l;

   /***************************Se abre el Archivo Fuente******************/
   if (argc == 1)
   {
      printf("Debe ingresar el nombre del archivo fuente (en lenguaje Micro) en la linea de comandos\n");
      return -1;
   }
   if (argc != 2)
   {
      printf("Numero incorrecto de argumentos\n");
      return -1;
   }
   strcpy(nomArchi, argv[1]);
   l = strlen(nomArchi);
   if (l > TAMNOM)
   {
      printf("Nombre incorrecto del Archivo Fuente\n");
      return -1;
   }
   if (nomArchi[l - 1] != 'm' || nomArchi[l - 2] != '.')
   {
      printf("Nombre incorrecto del Archivo Fuente\n");
      return -1;
   }

   if ((in = fopen(nomArchi, "r")) == NULL)
   {
      printf("No se pudo abrir archivo fuente\n");
      return -1;
   }

   /*************************Inicio Compilacion***************************/

   Objetivo();

   /**************************Se cierra el Archivo Fuente******************/

   fclose(in);

   return 0;
}

/**********Procedimientos de Analisis Sintactico (PAS) *****************/

void Objetivo(void)
{
   /* <objetivo> -> <programa> FDT #terminar */

   Programa();
   Match(FDT);
   Terminar();
}

void Programa(void)
{
   /* <programa> -> #comenzar INICIO <listaSentencias> FIN */

   Comenzar();
   Match(INICIO); //El objetivo de match es verificar que el token que le sigue al que se le pasa por parametro
    //(en este caso inicio ya que recien comienza a analizar el código), cumpla con la estructura de la gramática
    // Para ver el proceso ir a linea 406.
   ListaSentencias();
   Match(FIN);

}

void ListaSentencias(void)
{
   /* <listaSentencias> -> <sentencia> {<sentencia>} */
   // Chat: Procesa un conjunto de sentencias y se asegura que esten
   // correctamente formateadas y estructuradas según la gramática
   Sentencia();

   while (1)
   {
      switch (ProximoToken())
      {
      case ID:
      case LEER:
      case ESCRIBIR:
         Sentencia();
         break;
      default:
         return;
      }
   }
}

void Sentencia(void)
{
   // Chat: Analiza una sentencia, dependiendo de cual sea, llama otras funciones 
   // oara oricesar partes de esa sentencia
   TOKEN tok = ProximoToken();
   REG_EXPRESION izq, der;

   switch (tok)
   {
   case ID: /* <sentencia> -> ID := <expresion> #asignar ; */
      Identificador(&izq);
      Match(ASIGNACION);
      Expresion(&der);
      Asignar(izq, der);
      Match(PUNTOYCOMA);
      break;
   case LEER: /* <sentencia> -> LEER ( <listaIdentificadores> ) */
      Match(LEER);
      Match(PARENIZQUIERDO);
      ListaIdentificadores();
      Match(PARENDERECHO);
      Match(PUNTOYCOMA);
      break;
   case ESCRIBIR: /* <sentencia> -> ESCRIBIR ( <listaExpresiones> ) */
      Match(ESCRIBIR);
      Match(PARENIZQUIERDO);
      ListaExpresiones();
      Match(PARENDERECHO);
      Match(PUNTOYCOMA);
      break;
   default:
      return;
   }
}

void ListaIdentificadores(void)
{
   /* <listaIdentificadores> -> <identificador> #leer_id {COMA <identificador> #leer_id} */
   // Chat: Procesa una lista de identificadores
   TOKEN t;
   REG_EXPRESION reg;

   Identificador(&reg);
   Leer(reg);

   for (t = ProximoToken(); t == COMA; t = ProximoToken())
   {
      Match(COMA);
      Identificador(&reg);
      Leer(reg);
   }
}

void Identificador(REG_EXPRESION *presul)
{
   /* <identificador> -> ID #procesar_id */
   // Chat: Procesa un identificador para verificar si es correcta 
   // y registra su uso en la tabla de simbolos
   Match(ID);
   *presul = ProcesarId();
}

void ListaExpresiones(void)
{
   /* <listaExpresiones> -> <expresion> #escribir_exp {COMA <expresion> #escribir_exp} */
   // Chat: Procesa expresiones y se asegura que sean validas.
   TOKEN t;
   REG_EXPRESION reg;

   Expresion(&reg);
   Escribir(reg);

   for (t = ProximoToken(); t == COMA; t = ProximoToken())
   {
      Match(COMA);
      Expresion(&reg);
      Escribir(reg);
   }
}

void Expresion(REG_EXPRESION *presul)
{
   /* <expresion> -> <primaria> { <operadorAditivo> <primaria> #gen_infijo } */
   // Chat: Analiza una expresión, dependiende de que "operación" sea la procesa segun reglas gramaticales y aritmeticas
   REG_EXPRESION operandoIzq, operandoDer;
   char op[TAMLEX];
   TOKEN t;

   Primaria(&operandoIzq);

   for (t = ProximoToken(); t == SUMA || t == RESTA; t = ProximoToken())
   {
      OperadorAditivo(op);
      Primaria(&operandoDer);
      operandoIzq = GenInfijo(operandoIzq, op, operandoDer);
   }
   *presul = operandoIzq;
}

void Primaria(REG_EXPRESION *presul)
{
   // Chat: Procesa y maneja las partes mas basicas de una expresion
   TOKEN tok = ProximoToken();
   switch (tok)
   {
   case ID: /* <primaria> -> <identificador> */
      Identificador(presul);
      break;
   case CONSTANTE: /* <primaria> -> CONSTANTE #procesar_cte */
      Match(CONSTANTE);
      *presul = ProcesarCte();
      break;
   case PARENIZQUIERDO: /* <primaria> -> PARENIZQUIERDO <expresion> PARENDERECHO */
      Match(PARENIZQUIERDO);
      Expresion(presul);
      Match(PARENDERECHO);
      break;
   default:
      return;
   }
}

void OperadorAditivo(char *presul)
{
   /* <operadorAditivo> -> SUMA #procesar_op | RESTA #procesar_op */
   // Chat: Identifica y maneja los operadores aritmeticos de la expresión
   TOKEN t = ProximoToken();

   if (t == SUMA || t == RESTA)
   {
      Match(t);
      strcpy(presul, ProcesarOp());
   }
   else
      ErrorSintactico();
}

/**********************Rutinas Semanticas******************************/

REG_EXPRESION ProcesarCte(void)
{
   /* Convierte cadena que representa numero a numero entero y construye un registro semantico */
   // Chat: Es decir, registra el valor y tipo de constante para el analisis semantico
   REG_EXPRESION reg;

   reg.clase = CONSTANTE;
   strcpy(reg.nombre, buffer);
   sscanf(buffer, "%d", &reg.valor);

   return reg;
}

REG_EXPRESION ProcesarId(void)
{
   /* Declara ID y construye el correspondiente registro semantico */
   // Chat: Es decir, procesa, registra y valida identificadores
   REG_EXPRESION reg;

   Chequear(buffer);
   reg.clase = ID;
   strcpy(reg.nombre, buffer);

   return reg;
}

char *ProcesarOp(void)
{
   /* Declara OP y construye el correspondiente registro semantico */
   // Chat: Procesa los operadores en la expresiones del fuente (.txt).
   return buffer;
}

void Leer(REG_EXPRESION in)
{
   /* Genera la instruccion para leer */
   // Arma el formato del texto a mostrar por pantalla 
   Generar("Read", in.nombre, "Entera", "");
}

void Escribir(REG_EXPRESION out)
{
   /* Genera la instruccion para escribir */
   // Arma el formato del texto a mostrar por pantalla (otro caso)
   Generar("Write", Extraer(&out), "Entera", "");
}

REG_EXPRESION GenInfijo(REG_EXPRESION e1, char *op, REG_EXPRESION e2)
{
   /* Genera la instruccion para una operacion infija y construye un registro semantico con el resultado */
   // Chat: Genera código para operaciones infijas (como a + b, donde el simbolo esta entre los operadores)
   // Esto lo hace produciendo instrucciones intermedias para operaciones de este tipo
   REG_EXPRESION reg;
   static unsigned int numTemp = 1;
   char cadTemp[TAMLEX] = "Temp&";
   char cadNum[TAMLEX];
   char cadOp[TAMLEX];

   if (op[0] == '-')
      strcpy(cadOp, "Restar");
   if (op[0] == '+')
      strcpy(cadOp, "Sumar");

   sprintf(cadNum, "%d", numTemp);
   numTemp++;
   strcat(cadTemp, cadNum);

   if (e1.clase == ID)
      Chequear(Extraer(&e1));
   if (e2.clase == ID)
      Chequear(Extraer(&e2));
   Chequear(cadTemp);
   Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);

   strcpy(reg.nombre, cadTemp);

   return reg;
}
/***************Funciones Auxiliares**********************************/

void Match(TOKEN t)
{
   if (!(t == ProximoToken())) ErrorSintactico(); //Si el proximo token no cumple la estructura entonces imprime error sintatico y continua
   // T es el token buscado en la gramatica (el que sería correcto) y el que devuelve ProximoToken() sería la proxima linea o token
   // del archivo, si no coinciden hay error sintactico.
   flagToken = 0; //Esto es por si hay un error.
}

TOKEN ProximoToken()
{
   if (!flagToken) //Si flag es 0 o false entra al if.
   {
      tokenActual = scanner(); //Con esto se obtiene el proximo token del archivo
      if (tokenActual == ERRORLEXICO) ErrorLexico(); //Muestra error lexico
      flagToken = 1; //Supongo que no hay error entonces el flag lo establezco en 0 y 
      // el proximo token a analizar se verifica si hay error o no.
      if (tokenActual == ID)
      {
         Buscar(buffer, TS, &tokenActual);
      }
   }

   return tokenActual;
}

void ErrorLexico()
{
   printf("Error Lexico\n");
}

void ErrorSintactico()
{
   printf("Error Sintactico\n");
}

void Generar(char *co, char *a, char *b, char *c)
{
   /* Produce la salida de la instruccion para la MV por stdout */
   // Genera el texto a mostrar pantalla por medio de los parametros enviados
   // esto lo hace formateando los valores con el formato del printf.
   
   // Basicamente arma el texto asi (sin exagerar es este formato): Valor1 Valor2,Valor3,Valor4. 
   printf("%s %s%c%s%c%s\n", co, a, ',', b, ',', c);
}

char *Extraer(REG_EXPRESION *preg)
{
   /* Retorna la cadena del registro semantico */
   return preg->nombre;
}

int Buscar(char *id, RegTS *TS, TOKEN *t)
{
   /* Determina si un identificador esta en la TS */
   // TS = Tabla de simbolos
   int i = 0;

   while (strcmp("$", TS[i].identifi))
   {
      if (!strcmp(id, TS[i].identifi))
      {
         *t = TS[i].t;
         return 1;
      }
      i++;
   }
   return 0;
}

void Colocar(char *id, RegTS *TS)
{
   /* Agrega un identificador a la TS */

   int i = 4;

   while (strcmp("$", TS[i].identifi))
      i++;

   if (i < 999)
   {
      strcpy(TS[i].identifi, id);
      TS[i].t = ID;
      strcpy(TS[++i].identifi, "$");
   }
}

void Chequear(char *s)
{
   /* Si la cadena No esta en la Tabla de Simbolos la agrega,
      y si es el nombre de una variable genera la instruccion */

   TOKEN t;

   if (!Buscar(s, TS, &t))
   {
      Colocar(s, TS);
      Generar("Declara", s, "Entera", "");
   }
}

void Comenzar(void)
{
   /* Inicializaciones Semanticas */
}

void Terminar(void)
{
   /* Genera la instruccion para terminar la ejecucion del programa */

   Generar("Detiene", "", "", "");
}

void Asignar(REG_EXPRESION izq, REG_EXPRESION der)
{
   /* Genera la instruccion para la asignacion */

   Generar("Almacena", Extraer(&der), izq.nombre, "");
}

/**************************Scanner************************************/

TOKEN scanner() //Lector de tokens del archivo (extrae cada linea) y analiza cual es cada uno con un automata implementado (como ya lo hicimos en el primer tp)
{
   //Fila representa un elemento de la gramatica o del enum de los tokens (id, suma, resta, etc).
   // Y la columna a que estado va, para cada caso se acciona distinto ya que hay que verificar que significa 
   // pero siempre retorna un token (un numero o su alias)

   int tabla[NUMESTADOS][NUMCOLS] = {{1, 3, 5, 6, 7, 8, 9, 10, 11, 14, 13, 0, 14},
                                     {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
                                     {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14}};
   int car;
   int col;
   int estado = 0;

   int i = 0;

   do
   {
      car = fgetc(in);
      col = columna(car);
      estado = tabla[estado][col];

      if (col != 11)
      {
         buffer[i] = car;
         i++;
      }
   } while (!estadoFinal(estado) && !(estado == 14));

   buffer[i] = '\0';

   switch (estado)
   {
   case 2:
      if (col != 11)
      {
         ungetc(car, in);
         buffer[i - 1] = '\0';
      }
      return ID;
   case 4:
      if (col != 11)
      {
         ungetc(car, in);
         buffer[i - 1] = '\0';
      }
      return CONSTANTE;
   case 5:
      return SUMA;
   case 6:
      return RESTA;
   case 7:
      return PARENIZQUIERDO;
   case 8:
      return PARENDERECHO;
   case 9:
      return COMA;
   case 10:
      return PUNTOYCOMA;
   case 12:
      return ASIGNACION;
   case 13:
      return FDT;
   case 14:
      return ERRORLEXICO;
   }
}
int estadoFinal(int e)
{
   if (e == 0 || e == 1 || e == 3 || e == 11 || e == 14)
      return 0;
   return 1;
}
int columna(int c)
{
   if (isalpha(c))
      return 0;
   if (isdigit(c))
      return 1;
   if (c == '+')
      return 2;
   if (c == '-')
      return 3;
   if (c == '(')
      return 4;
   if (c == ')')
      return 5;
   if (c == ',')
      return 6;
   if (c == ';')
      return 7;
   if (c == ':')
      return 8;
   if (c == '=')
      return 9;
   if (c == EOF)
      return 10;
   if (isspace(c))
      return 11;
   return 12;
}
/*************Fin Scanner**********************************************/
