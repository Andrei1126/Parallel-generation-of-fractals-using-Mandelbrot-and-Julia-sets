/*
	Micut Andrei-Ion
	Grupa 331CB
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define PMAX 1024
#define ALGOS_NUM 2

static char *in_filename_julia;
static char *in_filename_mandelbrot;
static char *out_filename_julia;
static char *out_filename_mandelbrot;
static int P;

enum ALGOS_IDXES {
	JULIA = 0,
	MANDELBROT,
};

// structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

// structura pentru stocarea informatiilor necesare unui thread
struct thread_info {
	params *par[ALGOS_NUM];
	int **result[ALGOS_NUM];
	int height[ALGOS_NUM];
	int width[ALGOS_NUM];
	int tid;
	pthread_barrier_t *barrier;
};

// citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 6) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
	P = atoi(argv[5]);
}

// citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

// aloca memorie pentru rezultat
int **allocate_memory(int width, int height)
{
	int **result;
	int i;

	result = malloc(height * sizeof(int*));
	if (result == NULL) {
		printf("Eroare la malloc!\n");
		exit(1);
	}

	for (i = 0; i < height; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	return result;
}

// elibereaza memoria alocata
void free_memory(int **result, int height)
{
	int i;

	for (i = 0; i < height; i++) {
		free(result[i]);
	}
	free(result);
}

// functie ce interschimba doi pointeri
void swap_ptrs(int **a, int **b)
{
	int *aux;

	aux = *a;
	*a = *b;
	*b = aux;
}

// returneaza minimul dintre doua valori
int min(int a, int b)
{
	if (a < b) {
		return a;
	}

	return b;
}

// functie apelata de un thread pentru a inversa o portiune din matrice
void flip_result_matrix(int **result, int height, int tid)
{
	int start = (height / 2) * tid / P;
	int end = min((height / 2) * (tid + 1) / P, height);
	int i;

	// transforma rezultatul din coordonate matematice in coordonate ecran
	for (i = start; i < end; i++) {
		swap_ptrs(&result[i], &result[height - i - 1]);
	}
}

// ruleaza algoritmul Julia
void run_julia(void *arg)
{
	struct thread_info *info = (struct thread_info *)arg;
	params *par = info->par[JULIA];
	int **result = info->result[JULIA];
	int width = info->width[JULIA];
	int height = info->height[JULIA];
	int start = ceil((double)height / P) * info->tid;
    int end = min(ceil((double)height / P) * (info->tid + 1), height);
	int w, h;

	for (h = start; h < end; h++) {
		for (w = 0; w < width; w++) {
			int step = 0;
			complex z = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2) - pow(z_aux.b, 2) + par->c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par->c_julia.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	pthread_barrier_wait(info->barrier);
	flip_result_matrix(result, height, info->tid);
}

// ruleaza algoritmul Mandelbrot
void run_mandelbrot(void *arg)
{
	struct thread_info *info = (struct thread_info *)arg;
	params *par = info->par[MANDELBROT];
	int **result = info->result[MANDELBROT];
	int width = info->width[MANDELBROT];
	int height = info->height[MANDELBROT];
	int start = ceil((double)height / P) * info->tid;
    int end = min(ceil((double)height / P) * (info->tid + 1), height);
    int w, h;

	for (h = start; h < end; h++) {
		for (w = 0; w < width; w++) {
			complex c = { .a = w * par->resolution + par->x_min,
							.b = h * par->resolution + par->y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (sqrt(pow(z.a, 2.0) + pow(z.b, 2.0)) < 2.0 && step < par->iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = pow(z_aux.a, 2.0) - pow(z_aux.b, 2.0) + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[h][w] = step % 256;
		}
	}

	// se asteapta ca toate thread-urile sa ajunga in acest punct, inainte de a incepe
	// inversarea matricei de pixeli
	pthread_barrier_wait(info->barrier);
	flip_result_matrix(result, height, info->tid);
}

// rutina de start a fiecarui thread
void *thread_wrapper_fn(void *arg)
{
	// se genereaza portiunea corespunzatoare a ambilor fractali
	run_julia(arg);
	run_mandelbrot(arg);

	return NULL;
}

int main(int argc, char *argv[])
{
	// vectori cu datele necesarii rularii algoritmilor de generare a
	// fractalilor; implementare cat mai configurabila in cazul adaugarii
	// unui nou fractal
	char **in_filenames[ALGOS_NUM] = {
		&in_filename_julia,
		&in_filename_mandelbrot,
	};
	char **out_filenames[ALGOS_NUM] = {
		&out_filename_julia,
		&out_filename_mandelbrot,
	};
	struct thread_info tinfo[PMAX];
	pthread_t tid[PMAX];
	int **result[ALGOS_NUM];
	int width[ALGOS_NUM], height[ALGOS_NUM];
	params par[ALGOS_NUM];
	pthread_barrier_t barrier;
	int i, j;
	int rc;

	// se citesc argumentele programului
	get_args(argc, argv);

	if (P > PMAX) {
		fprintf(stderr, "Valoare PMAX trebuie marita. ejneboon\n");
		exit(1);
	}

	rc = pthread_barrier_init(&barrier, NULL, P);
	if (rc) {
		fprintf(stderr, "Eroare la crearea barierei.\n");
		exit(1);
	}

	for (i = 0; i < ALGOS_NUM; i++) {
		read_input_file(*in_filenames[i], &par[i]);
		width[i] = (par[i].x_max - par[i].x_min) / par[i].resolution;
		height[i] = (par[i].y_max - par[i].y_min) / par[i].resolution;
		result[i] = allocate_memory(width[i], height[i]);
	}

	for (i = 0; i < P; i++) {
		for (j = 0; j < ALGOS_NUM; j++) {
			tinfo[i].par[j] = &par[j];
			tinfo[i].result[j] = result[j];
			tinfo[i].width[j] = width[j];
			tinfo[i].height[j] = height[j];
		}

		tinfo[i].tid = i;
		tinfo[i].barrier = &barrier;
	}

	for (i = 0; i < P; i++) {
		pthread_create(&tid[i], NULL, &thread_wrapper_fn, &tinfo[i]);
	}

	for (i = 0; i < P; i++) {
		pthread_join(tid[i], NULL);
	}

	for (i = 0; i < ALGOS_NUM; i++) {
		write_output_file(*out_filenames[i], result[i], width[i], height[i]);
		free_memory(result[i], height[i]);
	}

	pthread_barrier_destroy(&barrier);

	return 0;
}
