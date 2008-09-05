out_file = 'fastpc_input_matlab'

%read data points
load my_xy.txt

x = my_xy(:,1);
y = my_xy(:,2)
size_x_temp = size(x);
size_x = size_x_temp(1);

%read functions
fid=fopen('input');
a = {};
size_a = 0;

%build cell array of function defs
while 1
    tline = fgetl(fid);
    if ~ischar(tline),   break,   end
    a= [a tline];
    size_a = size_a + 1;
end
fclose(fid);

M = zeros(size_x, size_a);

%evaluate functions for each x value
for i=1:size_x
    for j=1:size_a
        temp=@(x) eval(a{j});
        M(i,j) = temp(x(i));
    end
end



M = M*M.'
y = M.'*y
for k=1:size_x
    M(k,:) = M(k,:)/y(k)
end

num2str(size_x)
num2str(size_a)
ofid = fopen(out_file,'w');
fprintf(ofid,'%d %d %d\n',[size_x,size_a,size_x*size_a]);
for i=1:size_x
    for j=1:size_a
        M(i,j)
        fprintf(ofid,'%d %d %f\n', [i-1, j-1, M(i,j)]);
    end
end
fclose(ofid);

%bang bang
!../Desktop/summerResearch08/fastpc/code_mps/fastpc .05 fastpc_input_matlab
load tomog_solution

%plot new points
big_fcn = ''
big_fcn = strcat(num2str(tomog_solution(1)), '*(', a(1), ')')
for j=2:size_a
   big_fcn = strcat(big_fcn, '+', num2str(tomog_solution(j)), '*(', a(j), ')')
end
x_coord=0:0.5:10;
final = @(x)eval(big_fcn{1});
%plot(x_coord,final(x_coord))
x
y
plot(x_coord,final(x_coord),x,y,'--rs','LineWidth',2,...
                'MarkerEdgeColor','k',...
                'MarkerFaceColor','g',...
                'MarkerSize',10)