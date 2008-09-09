#include <stdio.h>
#include <math.h>
#include <stdlib.h>

typedef struct Hit_Tracker{
  int index;
  int hits;
}hit_tracker;

typedef struct Entry_Node{
  struct Entry_Node *next;
  int row;
  int col;
  double val;  
}entry;

typedef struct List_Node{
  struct List_Node *next;
  hit_tracker track;
}list_node;


int gen_row(double angle, double x, double y, double step, int **array,int bound_x, int bound_y,double *total_density,list_node **start, int *list_length){

  double distance = 0;
  *total_density = 0;
  *start = malloc(sizeof(list_node));
  (*start)->next = NULL;
  (*start)->track.index = -1;
  list_node *current = *start;
  int last_x = -1;
  int last_y = -1;
  int cur_x = 0;
  int cur_y = 0;

  while(1){
    distance = distance + step;
    cur_x = (int)floor(distance*cos(angle) + x);
    cur_y = (int)floor(distance*sin(angle) + y);

    if(cur_x > bound_x -1 || cur_y > bound_y - 1 || cur_x < 0 || cur_y <0){
      break;
    }
    *total_density = array[cur_y][cur_x] + *total_density;
    if(cur_x == last_x && cur_y ==last_y){
      current->track.hits++;
    }else{
      if((*start)->track.index == -1){
	(*start)->track.index = cur_y*bound_x+cur_x;
	(*start)->track.hits =  1;
      }else{
	list_node *new = malloc(sizeof(list_node));
	new->next = NULL;
	new->track.index = cur_y*bound_x +cur_x;
	new->track.hits = 1;
	current->next = new;
	current = new;
	last_x = cur_x;
	last_y = cur_y;
	*list_length = *list_length + 1;
      } 
    }
  }
  return 0;
}
  

int scan(entry **current,int width, int height,double angle_start,double angle_finish,double angle_increment,double x_start,double x_finish,double x_increment, double y_start, double y_finish, double y_increment,int *rows,int *non_zeros,double step,int **image){

  double i,j,a_index;
  for(i = y_start; i<y_finish; i = i+ y_increment){
    for(j = x_start; j<x_finish; j = j+x_increment){
      for(a_index = angle_start; a_index<angle_finish; a_index = a_index + angle_increment){
	double density = 0;
	list_node *start;
	int list_length = 0;
	gen_row(a_index*3.141592653589793238462643/180.0, j, i, step, image,width,height,&density,&start,&list_length);
	if(list_length <1 || density == 0){
	  continue;
	}
	*rows = *rows + 1;
	int z  = 0;
	for(z = 0; z<list_length; z++){
	  *non_zeros = *non_zeros + 1;
	  if( (*current)->val == -1){
	    (*current)->val = (double)start->track.hits/density;
	    (*current)->row = *rows -1;
	    (*current)->col = start->track.index; 
	  }else{
	    entry *new = malloc(sizeof(entry));
	    new->val = (double)start->track.hits/density;
	    new->row = *rows -1;
	    new->col = start->track.index;
	    new->next = NULL;
	    (*current)->next = new;
	    *current = new;
	  }
	  list_node *temp = start;
	  start = start->next;
	  free(temp);
	}
      }
    }
  }
  return 0;
}

int main(int argc, char **argv){

  int width,height,i,j;

  if(argc <4 || argc >5){
    printf("usage: ./tomog <input_file_name> <output_file_name> <step_size> [optional <angle_increment_size>]\n");
  }
  FILE *input = fopen(argv[1],"r");
  if(input == NULL){
    printf("input file not found\n");
    return 1;
  }
  FILE *output = fopen(argv[2],"w");

  
  double step = atof(argv[3]);
  if(step == 0.0){
    printf("bad step size must be a double\n");
    return 1;
  }

  if(fscanf(input,"%d %d ",&width,&height) != 2){
    printf("invalid first line of input\n");
    return 1;
  }
  
  int **image = malloc(height *sizeof(int*));
  for(i = 0; i<height; i++){
    image[i] = malloc(width * sizeof(int));
  }

  for(i = 0; i<height; i++){
    for(j = 0; j<width; j++){
      if(fscanf(input,"%d ",&(image[i][j])) != 1){
	printf("invalid input in input file\n");
	return 1;
      }
    }
  }

  int non_zeros = 0;
  int rows = 0;
  double num_row_angles = (double)width/2.0;
  double row_increment = 180.0/num_row_angles;
  double num_col_angles = (double)height/2.0;
  double col_increment = 180.0/num_col_angles;
  if(argc == 5){
    double angle_increment = atof(argv[4]);
    if(angle_increment == 0.0){
      printf("bad angle increment size must be a double\n");
      return 1;
    }
    row_increment = angle_increment;
    col_increment = angle_increment;
  }
  entry *start = malloc(sizeof(entry));
  start->val = -1;
  start->next = NULL;
  entry *current = start;
  int list_length = 0;
  
  scan(&current,width,height,-90,90,row_increment,0,1,1,0,height,1,&rows,&non_zeros,step,image);
  scan(&current,width,height,90,270,row_increment,width,width+1,1,0,height,1,&rows,&non_zeros,step,image);
  scan(&current,width,height,0,180,col_increment,0,width,1,0,1,1,&rows,&non_zeros,step,image);
  scan(&current,width,height,-180,0,col_increment,0,width,1,height,height+1,1,&rows,&non_zeros,step,image);

  fprintf(output,"%d %d %d\n",rows+(width*height),width*height,non_zeros+(width*height));
	  /*fprintf(output,"%d %d %d\n",rows,width*height,non_zeros); //original-- w/o 256-constraints */
  current = start;  

  int last_row = 0;
  while(1){
    fprintf(output,"%d %d %lf\n",current->row,current->col,current->val);
    last_row = current->row;
    if(current->next == NULL){
      break;
    }else{
      current = current->next;
    }
  }

	  /*take these out in final implementation; they are added to reproduce erratic behavior of alg */
  int total_vars = width*height;
  for (i=0; i < total_vars; i++) {
    last_row++;
    fprintf(output, "%d %d %lf\n", last_row, i, 255);
  }
  return 0;
}
