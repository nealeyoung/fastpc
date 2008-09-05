out_file = 'fastpc_input_matlab'

%read data points
load my_xy.txt

x = my_xy(:,1);
y = my_xy(:,2);
y_org = y;
size_x_temp = size(x);
size_x = size_x_temp(1);

%read functions
fid=fopen('input');
funcs = {};
size_funcs = 0;

%build cell array of function defs
while 1
    tline = fgetl(fid);
    if ~ischar(tline),   break,   end
      funcs{size_funcs +1}= @(x)(eval(tline));
    size_funcs = size_funcs + 1;
end
fclose(fid);

M = zeros(size_x, size_funcs);

%evaluate functions for each x value
for i=1:size_x
    for j=1:size_funcs
       M(i,j) = funcs{j}(x(i));
    end
end


%set up lsf problem: (M'M)c = M'y 
%    => for each row i, (M'M)(i)/(M'y)(i)*c <= 1; solve for c
M = M*M.';
y = M.'*y;
for k=1:size_x
	M(k,:) = M(k,:)/y(k);
end

%output constraints in fastpc format
ofid = fopen(out_file,'w');
fprintf(ofid,'%d %d %d\n',[size_x,size_funcs,size_x*size_funcs]);
for i=1:size_x
    for j=1:size_funcs
	    M(i,j);
        fprintf(ofid,'%d %d %f\n', [i-1, j-1, M(i,j)]);
    end
end
fclose(ofid);

%run fastpc
system('../fastpc .05 fastpc_input_matlab');
load tomog_solution

%build solution function as linear combo of input functions
big_func = @(x)0
for i=1:size_funcs
	big_func = @(x)tomog_solution(i)*funcs{i}(x) + big_func(x);
end

%graph solution function and original points
%x_cor=0:0.5:10;
x_cor = x;
x
y_org
y_cor=big_func(x_cor);
plot(x_cor,y_cor,'r',x,y_org,'g')
