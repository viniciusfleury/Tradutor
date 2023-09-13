#include<stdio.h>
#include <string.h>
#include <stdlib.h>

//struct para guardar endereço e registro de variavéis
typedef struct var {
	int ed;
	char reg[20];
}VAR;
//struct para guardar endereço e registro de arrays
typedef struct vet {
	int size,ed;
	char reg[20];
}VET;
//struct para guardar detalhes dos parametros
typedef struct par {
	int tipo;
	VET vet;
	VAR var;
}PAR;
PAR par[4];
PAR def[6];
char delim[] = " ";

int bloco,used,qtif;

/*Funçaõ que verifica se o parametro e inteiro ou array
  e guarda o endereço do parametro na struct PAR*/
int parametros(char *line) {
	int tot=0;
	char * pt;
	pt=strtok(line,delim);
	pt=strtok(NULL,delim);
	while(pt!=NULL){
		//inteiro = 1, array = 2
		if (strncmp(pt, "pi", 2)) {
			par[tot].tipo=1;
			bloco+=4;
			par[tot].var.ed=bloco;
		}else if(strncmp(pt, "pa", 2)) {
			par[tot].tipo=2;
			while(bloco%8!=0) bloco++;
			bloco+=8;
			par[tot].vet.ed=bloco;
		}
		tot++;
		pt=strtok(NULL,delim);
	}
	return tot;
}

/*Função que transforma uma string em um numero,usada para 
converter o numero que acompanha o ci, vi, va, pi ou pa da bnf*/
int tonum(char * s) {
	int x=2,num=0,neg=0;
	if(s[x]=='-') {
		neg++,x++;
	}
	while(s[x]>='0' && s[x]<='9') {
		num*=10;
		num+=(s[x]-'0');
		x++;
	}
	if(neg) num*=-1;
	return num;
}

/*Função que salva um registrador*/
void salvapar(int n) {
	int i;
	for (i = 0; i < n; ++i) {
		if(par[i].tipo==1) {
			printf("  movl %s, -%d(%%rbp)\n",par[i].var.reg,par[i].var.ed);
		}else {
			printf("  movq %s, -%d(%%rbp)\n",par[i].vet.reg,par[i].vet.ed);
		}
	}
}

/*Função que recupera um registrador*/
void pegapar(int n) {
	int i;
	for (i = 0; i < n; ++i) {
		if(par[i].tipo==1) {
			printf("  movl -%d(%%rbp), %s\n",par[i].var.ed,par[i].var.reg);
		}else {
			printf("  movq -%d(%%rbp), %s\n",par[i].vet.ed,par[i].vet.reg);
		}
	}
}

int main() {
	char line[200];
	char *pt;
	FILE *f;
	f = fopen("teste", "r");
	int t=1;
	int declara=0,parc=0,vr=0;
	strcpy(par[0].vet.reg,"%rdi");
	strcpy(par[0].var.reg,"%edi");
	strcpy(par[1].vet.reg,"%rsi");
	strcpy(par[1].var.reg,"%esi");
	strcpy(par[2].vet.reg,"%rdx");
	strcpy(par[2].var.reg,"%edx");
    
    //Percorrer o arquivo linha por linha
	while(fgets(line, sizeof(line), f) != NULL) {
		if (strncmp(line, "enddef", 6) == 0) {  //verifica se a linha tem um enddef
			declara=0;
		    printf("  subq $%d, %%rsp\n",used); //calcula o tamanho para alocar a pilha
	        continue;
		}
		else if (strncmp(line, "function", 8) == 0) { //verifica se a linha tem uma função
			bloco=0,used=0,qtif=0;
			printf(".text\n.globl f%d\nf%d:\n",t,t); //começo a primeira função
			t++; //interador para caso tenha mais funções
			printf("  pushq %%rbp\n  movq %%rsp, %%rbp\n");
			
			parc=parametros(line);
			while(bloco>used) used+=16; //faz com que a pilha seja multipla de 16
			continue;
		}
		else if (strncmp(line, "def", 3) == 0) { //verifica se a linha tem um def
			declara=1;
			vr=0;
	       continue;
		}
		else if (strncmp(line, "end", 3) == 0 && strncmp(line, "endif", 5) != 0) {
	       continue;
		}
		else if(declara){
			if (strncmp(line, "var", 3) == 0) { //verifica se a linha e uma variavel
				def[vr].tipo=1;
				bloco+=4; //aloca espaçõ na pilha para a variavel inteira
				def[vr].var.ed=bloco; //guarda o endereço da variavel
				while(bloco>used) used+=16;
			}else if(strncmp(line, "vet", 3) == 0) { //verifica se a linha e um array
				def[vr].tipo=2;
				pt=strtok(line,delim);
				pt=strtok(NULL,delim);
				pt=strtok(NULL,delim);
				pt=strtok(NULL,delim);
				int x=2;
				def[vr].vet.size=tonum(pt);
				bloco+=4*def[vr].vet.size;
				def[vr].vet.ed=bloco;
				while(bloco>used) used+=16;
			}
			vr++;
		}else {
			if (strncmp(line, "set", 3) == 0) { //verifica se a linha e um set
				pt=strtok(line,delim);
				int idx=0,settipo,ed,ved;
				// se o set for para um ponteiro settipo=2 senao settipo=1
				pt = strtok(NULL, delim);
				if(pt[0]=='v') { //verifica se o primeiro argumento do set e uma variavel
					settipo=1;
					ed=def[tonum(pt)-1].vet.ed;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					idx=tonum(pt);
					ed-=4*idx;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					if(pt[0]=='v') { //verifica se o segundo argumento e uma variavel
						ved=def[tonum(pt)-1].var.ed;
						printf("  movl -%d(%%rbp), %%r8d\n",ved);
						printf("  movl %%r8d, -%d(%%rbp)\n",ed);
					}else if(pt[0]=='c') { //verifica se o segundo argumento e uma constante
						ved=tonum(pt);
						printf("  movl $%d, -%d(%%rbp)\n",ved,ed);
					}
				}else if(pt[0]=='p') { //verifica se o primeiro argumento e um array
					int pos=tonum(pt)-1;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					idx=tonum(pt);
					ed=4*idx;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					if(pt[0]=='v') { //verifica se o segundo argumento e uma variavel
						ved=def[tonum(pt)-1].var.ed;
						printf("  movl -%d(%%rbp), %%r8d\n",ved);
						printf("  movl %%r8d, %d(%s)\n",ed,par[pos].vet.reg);
					}else if(pt[0]=='c') { //verifica se o segundo argumento e uma constante
						ved=tonum(pt);
						printf("  movl $%d, %d(%s)\n",ved,ed,par[pos].vet.reg);
					}
				}
			}else if (strncmp(line, "get", 3) == 0) { //verifica se a linha e um get
				pt=strtok(line,delim);
				int idx=0,settipo,ed,ved;

				pt = strtok(NULL, delim);
				if(pt[0]=='v') { //verifica se o primeiro argumento do get e uma variavel
					settipo=1;
					ed=def[tonum(pt)-1].vet.ed;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					idx=tonum(pt);
					ed-=4*idx;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl -%d(%%rbp), %%r8d\n",ed);
					printf("  movl %%r8d, -%d(%%rbp)\n",ved);
				}else if(pt[0]=='p') { //verifica se o primeiro argumento e um array
					int pos=tonum(pt)-1;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					idx=tonum(pt);
					ed=4*idx;
					pt = strtok(NULL, delim);
					pt = strtok(NULL, delim);
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl %d(%s), %%r8d\n",ed,par[pos].vet.reg);
					printf("  movl %%r8d, -%d(%%rbp)\n",ved);
				}
			}else if (strncmp(line, "if", 2) == 0) { //verifica se a linha e um if
				pt=strtok(line,delim);
				int ved;
				pt = strtok(NULL, delim);
				if(pt[0]=='v') { //verifica se o argumento do if e uma variavel
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl -%d(%%rbp), %%r8d\n",ved);
				}else if(pt[0]=='c') { //verifica se o argumento do if e uma constante
					ved=tonum(pt);
					printf("  movl $%d, %%r8d\n",ved);
				}
				printf("  cmpl $0, %%r8d\n");
				printf("  je endif%d\n",qtif);
			}else if (strncmp(line, "endif", 5) == 0) { //verifica se a linha e um endif
				printf("endif%d:\n",qtif);
				qtif++;
			}else if (strncmp(line, "return", 6) == 0) { //verifica se a linha e um return
				pt=strtok(line,delim);
				int ved;
				pt = strtok(NULL, delim);
				if(pt[0]=='v') { //verifica se o retorno e uma variavel
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl -%d(%%rbp), %%eax\n",ved);
				}else if(pt[0]=='c') { //verifica se o retorno e uma constante
					ved=tonum(pt);
					printf("  movl $%d, %%eax\n",ved);
				}
				printf("  leave\n  ret\n");
			}else if (strncmp(line, "return", 6) == 0) {
				pt=strtok(line,delim);
				int ved;
				pt = strtok(NULL, delim);
				if(pt[0]=='v') {
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl -%d(%%rbp), %%eax\n",ved);
				}else if(pt[0]=='c') {
					ved=tonum(pt);
					printf("  movl $%d, %%eax\n",ved);
				}
			}else if (strncmp(line, "return", 6) == 0) {
				pt=strtok(line,delim);
				int ved;
				pt = strtok(NULL, delim);
				if(pt[0]=='v') {
					ved=def[tonum(pt)-1].var.ed;
					printf("  movl -%d(%%rbp), %%eax\n",ved);
				}else if(pt[0]=='c') {
					ved=tonum(pt);
					printf("  movl $%d, %%eax\n",ved);
				}
			}else if (strncmp(line, "vi", 2) == 0) { //verifica se o segunto argumento e uma variavel
				pt=strtok(line,delim);
				int ved=def[tonum(pt)-1].var.ed;
				pt = strtok(NULL, delim);
				pt = strtok(NULL, delim);
				if(strncmp(pt, "call", 4) == 0) { //verifica chamada de função
					salvapar(parc);
					pt = strtok(NULL, delim);
					int id=(pt[0]-'0');
					int pr=0;
					pt = strtok(NULL, delim);
					while(pt!=NULL) {
						if (strncmp(pt, "vi", 2) == 0) { //verifica parametro como variavel
							printf("  movl -%d(%%rbp), %s\n",def[tonum(pt)-1].var.ed,par[pr].var.reg);
						}else if (strncmp(pt, "ci", 2) == 0) { //verifica parametro como constante
							printf("  movl $%d, %s\n",tonum(pt),par[pr].var.reg);
						}else if (strncmp(pt, "va", 2) == 0) { //verifica parametro como array
							printf("  leaq -%d(%%rbp), %s\n",def[tonum(pt)-1].vet.ed,par[pr].vet.reg);
						}
						pt = strtok(NULL, delim);
						pr++;
					}
					printf("  call f%d\n",id);
					printf("  movl %%eax, -%d(%%rbp)\n",ved);
					pegapar(parc);
				}else {
					if (strncmp(pt, "vi", 2) == 0) { 
						printf("  movl -%d(%%rbp), %%r8d\n",def[tonum(pt)-1].var.ed);
					}else if (strncmp(pt, "ci", 2) == 0) {
						printf("  movl $%d, %%r8d\n",tonum(pt));
					}else if (strncmp(pt, "pi", 2) == 0) {
						printf("  movl %s, %%r8d\n",par[tonum(pt)-1].var.reg);
					}
					pt = strtok(NULL, delim);
					if(pt!=NULL) {
						char ch=pt[0];
						pt = strtok(NULL, delim);
						if(ch=='+') printf("  addl ");
						else if(ch=='-') printf("  subl ");
					    else if(ch=='*') printf("  imull ");
						if (strncmp(pt, "vi", 2) == 0) printf("-%d(%%rbp), ",def[tonum(pt)-1].var.ed);
						else if (strncmp(pt, "ci", 2) == 0) printf("$%d, ",tonum(pt));
						printf("%%r8d\n");
					}
					printf("  movl %%r8d, -%d(%%rbp)\n",ved);
				}
			}
		}
	}
	fclose(f);
	return 0;
}