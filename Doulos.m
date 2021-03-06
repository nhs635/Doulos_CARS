%%

clc; clear; close all; 

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% High-Resolution FLIM
% 
% v181227 drafted
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% options
image_view = true; % true : image drawing, false : image non-drawing                  
is_write = true; % true : image writing, false : image non-writing
is_medfilt = true; % true: image medfilt, false: image non-medfilt

%% INI Parameters

% Filename
filelist = dir;
for i = 1 : length(filelist)
    if (length(filelist(i).name) > 5)
        if strcmp(filelist(i).name(end-4:end),'.data')
            dfilenm = filelist(i).name(1:end-5);
        end
    end
end
clear filelist;

% Please check ini file in the folder
fid = fopen(strcat(dfilenm,'.ini'));
config = textscan(fid,'%s');
fclose(fid); 

flimIntensityRange = zeros(3,2);
flimLifetimeRange = zeros(3,2);
for i = 1 : length(config{:})
    
    for j = 1 : 3
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimIntensityRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimIntensityRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end  
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMax_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,2) = str2double(config{1}{i}(eq_pos+1:end));
        end
        if (strfind(config{1}{i},sprintf('flimLifetimeRangeMin_Ch%d',j)))
            eq_pos = strfind(config{1}{i},'=');
            flimLifetimeRange(j,1) = str2double(config{1}{i}(eq_pos+1:end));
        end    
    end
    
    if (strfind(config{1}{i},'flimLifetimeColorTable'))
        eq_pos = strfind(config{1}{i},'=');
        flimLifetimeColorTable = str2double(config{1}{i}(eq_pos+1:end));
    end    
     
    if (strfind(config{1}{i},'imageSize'))
        eq_pos = strfind(config{1}{i},'=');
        imageSize = str2double(config{1}{i}(eq_pos+1:end));
    end 
    if (strfind(config{1}{i},'imageAveragingFrames'))
        eq_pos = strfind(config{1}{i},'=');
        imageAveragingFrames = str2double(config{1}{i}(eq_pos+1:end));
    end     
end

% Size parameters
frameSize = imageSize * imageSize;

if (is_write), mkdir('scaled_image_matlab'); end

clear config eq_pos;

%% Colortable

color_list = {"gray","invgray","sepia","jet","parula","hot","fire",...
    "hsv","smart","bor","cool","gem","gfb","ice","lifetime2","vessel","hsv1"};

fname_imap_ch1 = 'C:\Users\BOP-FLIM\Desktop\ColorTable\flim_ch1.colortable';
fname_imap_ch2 = 'C:\Users\BOP-FLIM\Desktop\ColorTable\flim_ch2.colortable';
fname_imap_ch3 = 'C:\Users\BOP-FLIM\Desktop\ColorTable\flim_ch3.colortable';
fname_lmap = strcat('C:\Users\BOP-FLIM\Desktop\ColorTable\',...
                    color_list{flimLifetimeColorTable+1},'.colortable');
fname_irmap = 'C:\Users\BOP-FLIM\Desktop\ColorTable\jet.colortable';

fid = fopen(fname_imap_ch1,'r');
imap_ch1 = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_imap_ch2,'r');
imap_ch2 = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_imap_ch3,'r');
imap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_lmap,'r');
lmap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

fid = fopen(fname_irmap,'r');
irmap = reshape(fread(fid,'uint8'),[256 3])/255;
fclose(fid);

clear fid color_list fname_imap fname_lmap fname_irmap;                

%% FLIM Image

intensity_image = zeros(imageSize,imageSize,3);
lifetime_image = zeros(imageSize,imageSize,3);
merged_image = zeros(imageSize,imageSize,3,3);


flimIntensityRange = [0,0.3; 0,0.6; 0,0.1]; % 3 x 2
flimLifetimeRange = [2.0,5.2; 2.0,5.2; 1,5.5]; % 3 x 2 

% load data
fname = sprintf([dfilenm,'.data']);
fid = fopen(fname,'r');

for i = 1 : 3
    temp = fread(fid,frameSize,'float');
    intensity_image(:,:,i) = reshape(temp,[imageSize imageSize])';
end

for i = 1 : 3
    temp = fread(fid,frameSize,'float');
    lifetime_image(:,:,i) = reshape(temp,[imageSize imageSize])';
    if (is_medfilt)
        lifetime_image(:,:,i) = medfilt2(lifetime_image(:,:,i),[3 3],'symmetric');
    end
    
    temp_l = uint8(255*mat2gray(lifetime_image(:,:,i),[flimLifetimeRange(i,1) flimLifetimeRange(i,2)]));    
    temp_l = ind2rgb(temp_l,lmap);
    temp_i = mat2gray(intensity_image(:,:,i),[flimIntensityRange(i,1) flimIntensityRange(i,2)]);  
    merged_image(:,:,:,i) = temp_l .* repmat(temp_i,[1 1 3]);
   
end

fclose(fid);
save('result.mat','intensity_image','lifetime_image');
clear fid;

%% Visualization
  
if (image_view)

    figure(100);
    subplot(331); imagesc(intensity_image(:,:,1)); axis image; 
    caxis([flimIntensityRange(1,1) flimIntensityRange(1,2)]);
    colormap(imap_ch1); freezeColors;
    title(sprintf('Ch1 Intensity [%.1f %.1f]',flimIntensityRange(1,1),flimIntensityRange(1,2)));
    subplot(332); imagesc(intensity_image(:,:,2)); axis image;
    caxis([flimIntensityRange(2,1) flimIntensityRange(2,2)]);
    colormap(imap_ch2); freezeColors;
    title(sprintf('Ch2 Intensity [%.1f %.1f]',flimIntensityRange(2,1),flimIntensityRange(2,2)));
    subplot(333); imagesc(intensity_image(:,:,3)); axis image;
    caxis([flimIntensityRange(3,1) flimIntensityRange(3,2)]);
    colormap(imap); freezeColors;
    title(sprintf('Ch3 Intensity [%.1f %.1f]',flimIntensityRange(3,1),flimIntensityRange(3,2)));
    
    subplot(334); imagesc(lifetime_image(:,:,1)); axis image;
    caxis([flimLifetimeRange(1,1) flimLifetimeRange(1,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch1 Lifetime [%.1f %.1f]',flimLifetimeRange(1,1),flimLifetimeRange(1,2)));
    subplot(335); imagesc(lifetime_image(:,:,2)); axis image;
    caxis([flimLifetimeRange(2,1) flimLifetimeRange(2,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch2 Lifetime [%.1f %.1f]',flimLifetimeRange(2,1),flimLifetimeRange(2,2)));
    subplot(336); imagesc(lifetime_image(:,:,3)); axis image;
    caxis([flimLifetimeRange(3,1) flimLifetimeRange(3,2)]);
    colormap(lmap); freezeColors; 
    title(sprintf('Ch3 Lifetime [%.1f %.1f]',flimLifetimeRange(3,1),flimLifetimeRange(3,2)));

    subplot(337); imagesc(merged_image(:,:,:,1)); axis image; title('Ch1 Merged');
    subplot(338); imagesc(merged_image(:,:,:,2)); axis image; title('Ch2 Merged');
    subplot(339); imagesc(merged_image(:,:,:,3)); axis image; title('Ch3 Merged');

end


%% Intensity ratio

% load data
fname = sprintf([dfilenm,'.data']);
fid = fopen(fname,'r');

for i = 1 : 3
    temp = fread(fid,frameSize,'float');
    intensity_image(:,:,i) = reshape(temp,[imageSize imageSize])';
    if (is_medfilt)
        intensity_image(:,:,i) = medfilt2(intensity_image(:,:,i),[3 3],'symmetric');
    end
end

fclose(fid);
clear fid;

IntRatiorange = [0 0.8; 2 16; 0 12.5];
Ch1_Ch2_ratio = intensity_image(:,:,1)./intensity_image(:,:,2);
Ch2_Ch3_ratio = intensity_image(:,:,2)./intensity_image(:,:,3);
Ch1_Ch3_ratio = intensity_image(:,:,1)./intensity_image(:,:,3);

figure(50);
subplot(131); imagesc(Ch1_Ch2_ratio); caxis(IntRatiorange(1,:)); colormap jet; axis image;
subplot(132); imagesc(Ch2_Ch3_ratio); caxis(IntRatiorange(2,:)); colormap jet; axis image;
subplot(133); imagesc(Ch1_Ch3_ratio); caxis(IntRatiorange(3,:)); colormap jet; axis image;


%% Image Write

if (is_write)

    temp = uint8(255 * mat2gray(intensity_image(:,:,1),[flimIntensityRange(1,1) flimIntensityRange(1,2)])); 
    imwrite(temp,imap_ch1,sprintf('scaled_image_matlab/intensity_image_ch_1_avg_%d_[%.1f %.1f].bmp',...
    imageAveragingFrames,flimIntensityRange(1,1),flimIntensityRange(1,2)));

    temp = uint8(255 * mat2gray(intensity_image(:,:,2),[flimIntensityRange(2,1) flimIntensityRange(2,2)])); 
    imwrite(temp,imap_ch2,sprintf('scaled_image_matlab/intensity_image_ch_2_avg_%d_[%.1f %.1f].bmp',...
    imageAveragingFrames,flimIntensityRange(2,1),flimIntensityRange(2,2)));

    temp = uint8(255 * mat2gray(intensity_image(:,:,3),[flimIntensityRange(3,1) flimIntensityRange(3,2)])); 
    imwrite(temp,imap,sprintf('scaled_image_matlab/intensity_image_ch_3_avg_%d_[%.1f %.1f].bmp',...
    imageAveragingFrames,flimIntensityRange(3,1),flimIntensityRange(3,2)));
        
    for i = 1 : 3
        temp = uint8(255 * mat2gray(lifetime_image(:,:,i),[flimLifetimeRange(i,1) flimLifetimeRange(i,2)])); 
        imwrite(temp,lmap,sprintf('scaled_image_matlab/lifetime_image_ch_%d_avg_%d_[%.1f %.1f].bmp',...
            i,imageAveragingFrames,flimLifetimeRange(i,1),flimLifetimeRange(i,2)));
    end
    for i = 1 : 3
        temp = uint8(255 * merged_image(:,:,:,i));
        imwrite(temp,sprintf('scaled_image_matlab/merged_image_ch_%d_avg_%d_i[%.1f %.1f]_l[%.1f %.1f].bmp',...
            i,imageAveragingFrames,flimIntensityRange(i,1),flimIntensityRange(i,2),...
            flimLifetimeRange(i,1),flimLifetimeRange(i,2)));
    end
    
    temp = uint8(255 * mat2gray(Ch1_Ch2_ratio,[IntRatiorange(1,1) IntRatiorange(1,2)]));
    imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch1_Ch2[%.1f %.1f].bmp',...
    IntRatiorange(1,1),IntRatiorange(1,2)));
    
    temp = uint8(255 * mat2gray(Ch2_Ch3_ratio,[IntRatiorange(2,1) IntRatiorange(2,2)]));
    imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch2_Ch3[%.1f %.1f].bmp',...
    IntRatiorange(2,1),IntRatiorange(2,2)));

    temp = uint8(255 * mat2gray(Ch1_Ch3_ratio,[IntRatiorange(3,1) IntRatiorange(3,2)]));
    imwrite(temp,irmap,sprintf('scaled_image_matlab/intensity_ratio_Ch1_Ch3[%.1f %.1f].bmp',...
    IntRatiorange(3,1),IntRatiorange(3,2)));

end
