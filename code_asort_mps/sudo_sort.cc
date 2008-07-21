//find B

//find bound = B*eps/c

//then if Mij is is less than bound set it to zero

//else set Mij to min(bound,Mij)

//then bucket sort

// for each row in Mij

//set up 2*log(c/eps buckets)

//then go through each element
#include <list>
#include <cmath>
#include <iostream>

using namespace std;
typedef list <double> double_list;

int sudo_sort(double* array,double length, double eps, double b,int c){

  int num_buckets = (int)ceil(log2(c/eps))*2; //not sure about base 2

  double_list** buckets = (double_list**)malloc(sizeof(double_list*)*num_buckets);
  for(int i = 0; i<num_buckets; i++){
    buckets[i] =  new double_list();
  }

  
  for(int i = 0; i<length; i++){
    
    //not sure about base 2 and the round might be floor
    int index = (int)floor(log2(array[i])-log2(b)+log2(c/eps));
    buckets[index]->push_back(array[i]);
    
  }

  //print out the buckets and add them to list

  for(int i = 0; i<num_buckets; i++){
    
     for (list<double>::iterator x = buckets[i]->begin(); x != buckets[i]->end(); ++x){
       cout << (*x)<< " item ";   
      }
     cout <<"bucket \n";

  }

}


double  array_min(double* array, int length){

  double min_temp = array[0];
  for(int i = 1; i<length; i++){

    if (array[i] < min_temp)
      min_temp = array[i];

  }
  return min_temp;

}

double min(double a, double b){

  if (a <b) return a;
  else return  b;

}

double* convert_row(double* array, int length,double eps, double b,int c){


  double bound = b*eps/c;
  double replace = b*c/eps;
  for(int i = 1; i<length; i++){

    if (array[i] < bound ) 
      array[i] = 0;
    else 
      array[i]= min(array[i],replace);
  }
  return array;
}

int main(){


  double* my_array = (double*)malloc(3*sizeof(double));
  my_array[0] = 4;
  my_array[1] = 110;
  my_array[2] = 2;
  int length  =3;
  int c =  3;
  double eps = 0.1;

  //needs changed to the max min for all rows
  double b = array_min(my_array,length);
  my_array  = convert_row(my_array,length,eps,b,c);
  sudo_sort(my_array,length,eps,b,c);

}
  
