% output file in fastpc format
% note: space must be preserved at beginning of string
out_file = ' fastpc_input_matlab';

% read data points
load my_xy.txt

x = my_xy(:,1);
y = my_xy(:,2);
y_org = y;
size_x_temp = size(x);
size_x = size_x_temp(1);

% read functions
fid=fopen('input');
funcs = {};
size_funcs = 0;

% build cell array of function defs
while 1
    tline = fgetl(fid);
    if ~ischar(tline),   break,   end
      funcs{size_funcs +1}= @(x)(eval(tline));
    size_funcs = size_funcs + 1;
end
fclose(fid);
M = zeros(size_x, size_funcs);

% evaluate functions for each x value
for i=1:size_x
    for j=1:size_funcs
       M(i,j) = funcs{j}(x(i));
    end
end


% set up lsf problem: (M'M)c = M'y 
%    => for each row i, (M'M)(i)/(M'y)(i)*c <= 1; solve for c
MT = M.';
M = MT*M;
y = MT*y;
dims_M = size(M);
rows_M = dims_M(1);
cols_M = dims_M(2);
for k=1:rows_M
	M(k,:) = M(k,:)/y(k);
end

% output constraints in fastpc format
ofid = fopen(out_file,'w');
fprintf(ofid,'%d %d %d\n',[rows_M,cols_M,rows_M*cols_M]);
for i=1:rows_M
    for j=1:cols_M
	    M(i,j);
        fprintf(ofid,'%d %d %f\n', [i-1, j-1, M(i,j)]);
    end
end
fclose(ofid);

% run fastpc
system(strcat('../fastpc .05 ',out_file));

% build solution function as linear combo of input functions
load fastpc_solution;
sol = fastpc_solution;
%build solution function as linear combo of input functions
big_func = @(x)0
for i=1:size_funcs
	big_func = @(x)sol(i)*funcs{i}(x) + big_func(x);
end

% graph solution function and original points
x_cor = x;
y_cor=arrayfun(big_func,x_cor);
plot(x_cor,y_cor,'--rs',x,y_org,'g');

% calculate absolute squared error
ls = @(x,y) (y-x)^2;
ls_array = arrayfun(ls,y_org,y_cor);
ls_total = sum(ls_array)
