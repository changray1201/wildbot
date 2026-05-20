import torch
import torch.nn as nn
import torch.nn.functional as F
from ultralytics.nn.modules import Conv

def autopad(k, p=None, d=1):  # kernel, padding, dilation
    if d > 1:
        k = d * (k - 1) + 1 if isinstance(k, int) else [d * (x - 1) + 1 for x in k]
    if p is None:
        p = k // 2 if isinstance(k, int) else [x // 2 for x in k]
    return p

class GroupBatchnorm2d(nn.Module):
    def __init__(self, c_num: int, group_num: int = 16, eps: float = 1e-10):
        super(GroupBatchnorm2d, self).__init__()
        assert c_num >= group_num
        self.group_num = group_num
        self.gamma = nn.Parameter(torch.randn(c_num, 1, 1))
        self.beta = nn.Parameter(torch.zeros(c_num, 1, 1))
        self.eps = eps

    def forward(self, x):
        N, C, H, W = x.size()
        x = x.view(N, self.group_num, -1)
        mean = x.mean(dim=2, keepdim=True)
        std = x.std(dim=2, keepdim=True)
        x = (x - mean) / (std + self.eps)
        x = x.view(N, C, H, W)
        return x * self.gamma + self.beta

class SRU(nn.Module):
    def __init__(self, oup_channels: int, group_num: int = 16, gate_treshold: float = 0.5):
        super().__init__()
        self.gn = GroupBatchnorm2d(oup_channels, group_num=group_num)
        self.gate_treshold = gate_treshold
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        gn_x = self.gn(x)
        w_gamma = F.softmax(self.gn.gamma, dim=0)
        reweights = self.sigmoid(gn_x * w_gamma)
        info_mask = w_gamma > self.gate_treshold
        noninfo_mask = w_gamma <= self.gate_treshold
        x_1 = info_mask * reweights * x
        x_2 = noninfo_mask * reweights * x
        return self.reconstruct(x_1, x_2)

    def reconstruct(self, x_1, x_2):
        x_11, x_12 = torch.split(x_1, x_1.size(1) // 2, dim=1)
        x_21, x_22 = torch.split(x_2, x_2.size(1) // 2, dim=1)
        return torch.cat([x_11 + x_22, x_12 + x_21], dim=1)

class CRU(nn.Module):
    def __init__(self, op_channels: int, alpha: float = 1/2, squeeze_radio: int = 2, group_size: int = 2, g: int = 1, act: bool = True):
        super().__init__()
        self.up_channel = up_channel = int(alpha * op_channels)
        self.low_channel = low_channel = op_channels - up_channel
        self.squeeze1 = nn.Conv2d(up_channel, up_channel // squeeze_radio, kernel_size=1, bias=False)
        self.squeeze2 = nn.Conv2d(low_channel, low_channel // squeeze_radio, kernel_size=1, bias=False)
        
        # GWC: Group-wise Convolution
        self.GWC = nn.Conv2d(up_channel // squeeze_radio, op_channels, kernel_size=3, stride=1, padding=1, groups=group_size, bias=False)
        # PWC: Point-wise Convolution
        self.PWC = nn.Conv2d(low_channel // squeeze_radio, op_channels, kernel_size=1, bias=False)
        self.pool = nn.AdaptiveAvgPool2d(1)
        self.act = nn.SiLU() if act else nn.Identity()

    def forward(self, x):
        x_up, x_low = torch.split(x, [self.up_channel, self.low_channel], dim=1)
        x_up = self.squeeze1(x_up)
        x_low = self.squeeze2(x_low)
        
        y_gwc = self.GWC(x_up)
        y_pwc = self.PWC(x_low)
        
        # 融合特徵
        s_gwc = self.pool(y_gwc)
        s_pwc = self.pool(y_pwc)
        soft_mask = F.softmax(torch.cat([s_gwc, s_pwc], dim=1), dim=1)
        s1, s2 = torch.split(soft_mask, soft_mask.size(1) // 2, dim=1)
        
        return self.act(y_gwc * s1 + y_pwc * s2)

class SCConv(nn.Module):
    def __init__(self, c1, c2, k=1, s=1, p=None, g=1, d=1, act=True):
        super().__init__()
        # 為了簡化，這裡假設 c1 == c2，若不相等通常會加一個 Conv 調整
        self.conv = Conv(c1, c2, k, s, p, g, d, act) if (c1 != c2 or s != 1) else nn.Identity()
        self.sru = SRU(c2)
        self.cru = CRU(c2)

    def forward(self, x):
        x = self.conv(x)
        x = self.sru(x)
        x = self.cru(x)
        return x